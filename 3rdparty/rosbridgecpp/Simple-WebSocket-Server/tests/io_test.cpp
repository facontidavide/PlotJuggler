#include "assert.hpp"
#include "client_ws.hpp"
#include "server_ws.hpp"

using namespace std;

typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
typedef SimpleWeb::SocketClient<SimpleWeb::WS> WsClient;

int main() {
  WsServer server;
  server.config.port = 8080;
  server.config.thread_pool_size = 4;

  auto &echo = server.endpoint["^/echo/?$"];

  atomic<int> server_callback_count(0);

  echo.on_message = [&server_callback_count](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
    auto in_message_str = in_message->string();
    ASSERT(in_message_str == "Hello");

    ++server_callback_count;
    connection->send(in_message_str, [](const SimpleWeb::error_code &ec) {
      if(ec) {
        cerr << ec.message() << endl;
        ASSERT(false);
      }
    });
  };

  echo.on_open = [&server_callback_count](shared_ptr<WsServer::Connection> connection) {
    ++server_callback_count;
    ASSERT(!connection->remote_endpoint_address().empty());
    ASSERT(connection->remote_endpoint_port() > 0);
  };

  echo.on_close = [&server_callback_count](shared_ptr<WsServer::Connection> /*connection*/, int /*status*/, const string & /*reason*/) {
    ++server_callback_count;
  };

  echo.on_error = [](shared_ptr<WsServer::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
    cerr << ec.message() << endl;
    ASSERT(false);
  };

  auto &echo_thrice = server.endpoint["^/echo_thrice/?$"];
  echo_thrice.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
    auto out_message = make_shared<WsServer::OutMessage>();
    *out_message << in_message->string();

    connection->send(out_message, [connection, out_message](const SimpleWeb::error_code &ec) {
      if(!ec)
        connection->send(out_message);
    });
    connection->send(out_message);
  };

  auto &fragmented_message = server.endpoint["^/fragmented_message/?$"];
  fragmented_message.on_message = [&server_callback_count](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
    ++server_callback_count;
    ASSERT(in_message->string() == "fragmented message");

    connection->send("fragmented", nullptr, 1);
    connection->send(" ", nullptr, 0);
    connection->send("message", nullptr, 128);
  };

  thread server_thread([&server]() {
    server.start();
  });

  this_thread::sleep_for(chrono::seconds(1));

  // test stop
  server.stop();
  server_thread.join();

  server_thread = thread([&server]() {
    server.start();
  });

  this_thread::sleep_for(chrono::seconds(1));

  for(size_t i = 0; i < 400; ++i) {
    WsClient client("localhost:8080/echo");

    atomic<int> client_callback_count(0);
    atomic<bool> closed(false);

    client.on_message = [&](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
      ASSERT(in_message->string() == "Hello");

      ++client_callback_count;

      ASSERT(!closed);

      connection->send_close(1000);
    };

    client.on_open = [&](shared_ptr<WsClient::Connection> connection) {
      ++client_callback_count;

      ASSERT(!closed);

      connection->send("Hello");
    };

    client.on_close = [&](shared_ptr<WsClient::Connection> /*connection*/, int /*status*/, const string & /*reason*/) {
      ++client_callback_count;
      ASSERT(!closed);
      closed = true;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
      cerr << ec.message() << endl;
      ASSERT(false);
    };

    thread client_thread([&client]() {
      client.start();
    });

    while(!closed)
      this_thread::sleep_for(chrono::milliseconds(5));

    client.stop();
    client_thread.join();

    ASSERT(client_callback_count == 3);
  }
  ASSERT(server_callback_count == 1200);

  for(size_t i = 0; i < 400; ++i) {
    WsClient client("localhost:8080/echo_thrice");

    atomic<int> client_callback_count(0);
    atomic<bool> closed(false);

    client.on_message = [&](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
      ASSERT(in_message->string() == "Hello");

      ++client_callback_count;

      ASSERT(!closed);

      if(client_callback_count == 4)
        connection->send_close(1000);
    };

    client.on_open = [&](shared_ptr<WsClient::Connection> connection) {
      ++client_callback_count;

      ASSERT(!closed);

      connection->send("Hello");
    };

    client.on_close = [&](shared_ptr<WsClient::Connection> /*connection*/, int /*status*/, const string & /*reason*/) {
      ++client_callback_count;
      ASSERT(!closed);
      closed = true;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
      cerr << ec.message() << endl;
      ASSERT(false);
    };

    thread client_thread([&client]() {
      client.start();
    });

    while(!closed)
      this_thread::sleep_for(chrono::milliseconds(5));

    client.stop();
    client_thread.join();

    ASSERT(client_callback_count == 5);
  }

  {
    WsClient client("localhost:8080/echo");

    server_callback_count = 0;
    atomic<int> client_callback_count(0);
    atomic<bool> closed(false);

    client.on_message = [&](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
      ASSERT(in_message->string() == "Hello");

      ++client_callback_count;

      ASSERT(!closed);

      if(client_callback_count == 201)
        connection->send_close(1000);
    };

    client.on_open = [&](shared_ptr<WsClient::Connection> connection) {
      ++client_callback_count;

      ASSERT(!closed);

      for(size_t i = 0; i < 200; ++i) {
        auto send_stream = make_shared<WsClient::OutMessage>();
        *send_stream << "Hello";
        connection->send(send_stream);
      }
    };

    client.on_close = [&](shared_ptr<WsClient::Connection> /*connection*/, int /*status*/, const string & /*reason*/) {
      ++client_callback_count;
      ASSERT(!closed);
      closed = true;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
      cerr << ec.message() << endl;
      ASSERT(false);
    };

    thread client_thread([&client]() {
      client.start();
    });

    while(!closed)
      this_thread::sleep_for(chrono::milliseconds(5));

    client.stop();
    client_thread.join();

    ASSERT(client_callback_count == 202);
    ASSERT(server_callback_count == 202);
  }

  {
    WsClient client("localhost:8080/fragmented_message");

    server_callback_count = 0;
    atomic<int> client_callback_count(0);
    atomic<bool> closed(false);

    client.on_message = [&](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
      ASSERT(in_message->string() == "fragmented message");

      ++client_callback_count;

      connection->send_close(1000);
    };

    client.on_open = [&](shared_ptr<WsClient::Connection> connection) {
      ++client_callback_count;

      ASSERT(!closed);

      connection->send("fragmented", nullptr, 1);
      connection->send(" ", nullptr, 0);
      connection->send("message", nullptr, 128);
    };

    client.on_close = [&](shared_ptr<WsClient::Connection> /*connection*/, int /*status*/, const string & /*reason*/) {
      ASSERT(!closed);
      closed = true;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
      cerr << ec.message() << endl;
      ASSERT(false);
    };

    thread client_thread([&client]() {
      client.start();
    });

    while(!closed)
      this_thread::sleep_for(chrono::milliseconds(5));

    client.stop();
    client_thread.join();

    ASSERT(client_callback_count == 2);
    ASSERT(server_callback_count == 1);
  }

  server.stop();
  server_thread.join();
}
