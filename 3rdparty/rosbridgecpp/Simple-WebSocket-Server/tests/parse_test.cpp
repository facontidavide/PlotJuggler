#include "assert.hpp"
#include "client_ws.hpp"
#include "server_ws.hpp"
#include <iostream>

using namespace std;
using namespace SimpleWeb;

class SocketServerTest : public SocketServerBase<WS> {
public:
  SocketServerTest() : SocketServerBase<WS>::SocketServerBase(8080) {}

  void accept() {}

  void parse_request_test() {
    std::shared_ptr<Connection> connection(new Connection(handler_runner, 0, *io_service));

    ostream ss(&connection->read_buffer);
    ss << "GET /test/ HTTP/1.1\r\n";
    ss << "TestHeader: test\r\n";
    ss << "TestHeader2:test2\r\n";
    ss << "TestHeader3:test3a\r\n";
    ss << "TestHeader3:test3b\r\n";
    ss << "\r\n";

    std::istream stream(&connection->read_buffer);
    ASSERT(RequestMessage::parse(stream, connection->method, connection->path, connection->query_string, connection->http_version, connection->header));

    ASSERT(connection->method == "GET");
    ASSERT(connection->path == "/test/");
    ASSERT(connection->http_version == "1.1");

    ASSERT(connection->header.size() == 4);
    auto header_it = connection->header.find("TestHeader");
    ASSERT(header_it != connection->header.end() && header_it->second == "test");
    header_it = connection->header.find("TestHeader2");
    ASSERT(header_it != connection->header.end() && header_it->second == "test2");

    header_it = connection->header.find("testheader");
    ASSERT(header_it != connection->header.end() && header_it->second == "test");
    header_it = connection->header.find("testheader2");
    ASSERT(header_it != connection->header.end() && header_it->second == "test2");

    auto range = connection->header.equal_range("testheader3");
    auto first = range.first;
    auto second = first;
    ++second;
    ASSERT(range.first != connection->header.end() && range.second != connection->header.end() &&
           ((first->second == "test3a" && second->second == "test3b") ||
            (first->second == "test3b" && second->second == "test3a")));
  }
};

class SocketClientTest : public SocketClientBase<WS> {
public:
  SocketClientTest(const std::string &server_port_path) : SocketClientBase<WS>::SocketClientBase(server_port_path, 80) {}

  void connect() {}

  void constructor_parse_test1() {
    ASSERT(path == "/test");
    ASSERT(host == "test.org");
    ASSERT(port == 8080);
  }

  void constructor_parse_test2() {
    ASSERT(path == "/test");
    ASSERT(host == "test.org");
    ASSERT(port == 80);
  }

  void constructor_parse_test3() {
    ASSERT(path == "/");
    ASSERT(host == "test.org");
    ASSERT(port == 80);
  }

  void constructor_parse_test4() {
    ASSERT(path == "/");
    ASSERT(host == "test.org");
    ASSERT(port == 8080);
  }

  void parse_response_header_test() {
    auto connection = std::shared_ptr<Connection>(new Connection(handler_runner, config.timeout_idle, *io_service));
    connection->in_message = std::shared_ptr<InMessage>(new InMessage());

    ostream stream(&connection->in_message->streambuf);
    stream << "HTTP/1.1 200 OK\r\n";
    stream << "TestHeader: test\r\n";
    stream << "TestHeader2:test2\r\n";
    stream << "TestHeader3:test3a\r\n";
    stream << "TestHeader3:test3b\r\n";
    stream << "\r\n";

    ASSERT(ResponseMessage::parse(*connection->in_message, connection->http_version, connection->status_code, connection->header));

    ASSERT(connection->header.size() == 4);
    auto header_it = connection->header.find("TestHeader");
    ASSERT(header_it != connection->header.end() && header_it->second == "test");
    header_it = connection->header.find("TestHeader2");
    ASSERT(header_it != connection->header.end() && header_it->second == "test2");

    header_it = connection->header.find("testheader");
    ASSERT(header_it != connection->header.end() && header_it->second == "test");
    header_it = connection->header.find("testheader2");
    ASSERT(header_it != connection->header.end() && header_it->second == "test2");

    auto range = connection->header.equal_range("testheader3");
    auto first = range.first;
    auto second = first;
    ++second;
    ASSERT(range.first != connection->header.end() && range.second != connection->header.end() &&
           ((first->second == "test3a" && second->second == "test3b") ||
            (first->second == "test3b" && second->second == "test3a")));

    connection.reset();
  }
};

int main() {
  ASSERT(case_insensitive_equal("Test", "tesT"));
  ASSERT(case_insensitive_equal("tesT", "test"));
  ASSERT(!case_insensitive_equal("test", "tseT"));
  CaseInsensitiveEqual equal;
  ASSERT(equal("Test", "tesT"));
  ASSERT(equal("tesT", "test"));
  ASSERT(!equal("test", "tset"));
  CaseInsensitiveHash hash;
  ASSERT(hash("Test") == hash("tesT"));
  ASSERT(hash("tesT") == hash("test"));
  ASSERT(hash("test") != hash("tset"));

  SocketServerTest serverTest;
  serverTest.io_service = std::make_shared<io_context>();

  serverTest.parse_request_test();

  SocketClientTest clientTest("test.org:8080/test");
  clientTest.constructor_parse_test1();

  SocketClientTest clientTest2("test.org/test");
  clientTest2.constructor_parse_test2();

  SocketClientTest clientTest3("test.org");
  clientTest3.constructor_parse_test3();

  SocketClientTest clientTest4("test.org:8080");
  clientTest4.io_service = std::make_shared<io_context>();
  clientTest4.constructor_parse_test4();

  clientTest4.parse_response_header_test();
}
