#ifndef SIMPLE_WEB_SERVER_WS_HPP
#define SIMPLE_WEB_SERVER_WS_HPP

#include "asio_compatibility.hpp"
#include "crypto.hpp"
#include "mutex.hpp"
#include "utility.hpp"
#include <array>
#include <atomic>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <thread>
#include <unordered_set>

// Late 2017 TODO: remove the following checks and always use std::regex
#ifdef USE_BOOST_REGEX
#include <boost/regex.hpp>
namespace SimpleWeb {
  namespace regex = boost;
}
#else
#include <regex>
namespace SimpleWeb {
  namespace regex = std;
}
#endif

namespace SimpleWeb {
  template <class socket_type>
  class SocketServer;

  template <class socket_type>
  class SocketServerBase {
  public:
    class InMessage : public std::istream {
      friend class SocketServerBase<socket_type>;

    public:
      unsigned char fin_rsv_opcode;
      std::size_t size() noexcept {
        return length;
      }

      /// Convenience function to return std::string. The stream buffer is consumed.
      std::string string() noexcept {
        try {
          std::string str;
          auto size = streambuf.size();
          str.resize(size);
          read(&str[0], static_cast<std::streamsize>(size));
          return str;
        }
        catch(...) {
          return std::string();
        }
      }

    private:
      InMessage() noexcept : std::istream(&streambuf), length(0) {}
      InMessage(unsigned char fin_rsv_opcode, std::size_t length) noexcept : std::istream(&streambuf), fin_rsv_opcode(fin_rsv_opcode), length(length) {}
      std::size_t length;
      asio::streambuf streambuf;
    };

    /// The buffer is not consumed during send operations.
    /// Do not alter while sending.
    class OutMessage : public std::ostream {
      friend class SocketServerBase<socket_type>;

      asio::streambuf streambuf;

    public:
      OutMessage() noexcept : std::ostream(&streambuf) {}
      OutMessage(std::size_t capacity) noexcept : std::ostream(&streambuf) {
        streambuf.prepare(capacity);
      }

      /// Returns the size of the buffer
      std::size_t size() const noexcept {
        return streambuf.size();
      }
    };

    class Connection : public std::enable_shared_from_this<Connection> {
      friend class SocketServerBase<socket_type>;
      friend class SocketServer<socket_type>;

    public:
      Connection(std::unique_ptr<socket_type> &&socket_) noexcept : socket(std::move(socket_)), timeout_idle(0), closed(false) {}

      std::string method, path, query_string, http_version;

      CaseInsensitiveMultimap header;

      regex::smatch path_match;

      std::string remote_endpoint_address() noexcept {
        try {
          return socket->lowest_layer().remote_endpoint().address().to_string();
        }
        catch(...) {
        }
        return std::string();
      }

      unsigned short remote_endpoint_port() noexcept {
        try {
          return socket->lowest_layer().remote_endpoint().port();
        }
        catch(...) {
        }
        return 0;
      }

    private:
      template <typename... Args>
      Connection(std::shared_ptr<ScopeRunner> handler_runner_, long timeout_idle, Args &&... args) noexcept
          : handler_runner(std::move(handler_runner_)), socket(new socket_type(std::forward<Args>(args)...)), timeout_idle(timeout_idle), closed(false) {}

      std::shared_ptr<ScopeRunner> handler_runner;

      std::unique_ptr<socket_type> socket; // Socket must be unique_ptr since asio::ssl::stream<asio::ip::tcp::socket> is not movable

      asio::streambuf read_buffer;
      std::shared_ptr<InMessage> fragmented_in_message;

      long timeout_idle;

      Mutex timer_mutex;
      std::unique_ptr<asio::steady_timer> timer GUARDED_BY(timer_mutex);

      void close() noexcept {
        error_code ec;
        socket->lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        socket->lowest_layer().cancel(ec);
      }

      void set_timeout(long seconds = -1) noexcept {
        bool use_timeout_idle = false;
        if(seconds == -1) {
          use_timeout_idle = true;
          seconds = timeout_idle;
        }

        LockGuard lock(timer_mutex);

        if(seconds == 0) {
          timer = nullptr;
          return;
        }

        timer = std::unique_ptr<asio::steady_timer>(new asio::steady_timer(get_socket_executor(*socket), std::chrono::seconds(seconds)));
        std::weak_ptr<Connection> connection_weak(this->shared_from_this()); // To avoid keeping Connection instance alive longer than needed
        timer->async_wait([connection_weak, use_timeout_idle](const error_code &ec) {
          if(!ec) {
            if(auto connection = connection_weak.lock()) {
              if(use_timeout_idle)
                connection->send_close(1000, "idle timeout"); // 1000=normal closure
              else
                connection->close();
            }
          }
        });
      }

      void cancel_timeout() noexcept {
        LockGuard lock(timer_mutex);
        if(timer) {
          try {
            timer->cancel();
          }
          catch(...) {
          }
        }
      }

      class OutData {
      public:
        OutData(std::shared_ptr<OutMessage> out_header_, std::shared_ptr<OutMessage> out_message_,
                std::function<void(const error_code)> &&callback_) noexcept
            : out_header(std::move(out_header_)), out_message(std::move(out_message_)), callback(std::move(callback_)) {}
        std::shared_ptr<OutMessage> out_header;
        std::shared_ptr<OutMessage> out_message;
        std::function<void(const error_code)> callback;
      };

      Mutex send_queue_mutex;
      std::list<OutData> send_queue GUARDED_BY(send_queue_mutex);

      /// send_queue_mutex must be locked here
      void send_from_queue() REQUIRES(send_queue_mutex) {
        std::array<asio::const_buffer, 2> buffers{send_queue.begin()->out_header->streambuf.data(), send_queue.begin()->out_message->streambuf.data()};
        auto self = this->shared_from_this();
        asio::async_write(*socket, buffers, [self](const error_code &ec, std::size_t /*bytes_transferred*/) {
          auto lock = self->handler_runner->continue_lock();
          if(!lock)
            return;
          {
            LockGuard lock(self->send_queue_mutex);
            if(!ec) {
              auto it = self->send_queue.begin();
              auto callback = std::move(it->callback);
              self->send_queue.erase(it);
              if(self->send_queue.size() > 0)
                self->send_from_queue();

              lock.unlock();
              if(callback)
                callback(ec);
            }
            else {
              // All handlers in the queue is called with ec:
              std::vector<std::function<void(const error_code &)>> callbacks;
              for(auto &out_data : self->send_queue) {
                if(out_data.callback)
                  callbacks.emplace_back(std::move(out_data.callback));
              }
              self->send_queue.clear();

              lock.unlock();
              for(auto &callback : callbacks)
                callback(ec);
            }
          }
        });
      }

      std::atomic<bool> closed;

    public:
      /// fin_rsv_opcode: 129=one fragment, text, 130=one fragment, binary, 136=close connection.
      /// See http://tools.ietf.org/html/rfc6455#section-5.2 for more information.
      void send(const std::shared_ptr<OutMessage> &out_message, const std::function<void(const error_code &)> &callback = nullptr, unsigned char fin_rsv_opcode = 129) {
        cancel_timeout();
        set_timeout();

        std::size_t length = out_message->size();

        auto out_header = std::make_shared<OutMessage>(10); // Header is at most 10 bytes

        out_header->put(static_cast<char>(fin_rsv_opcode));
        // Unmasked (first length byte<128)
        if(length >= 126) {
          std::size_t num_bytes;
          if(length > 0xffff) {
            num_bytes = 8;
            out_header->put(127);
          }
          else {
            num_bytes = 2;
            out_header->put(126);
          }

          for(std::size_t c = num_bytes - 1; c != static_cast<std::size_t>(-1); c--)
            out_header->put((static_cast<unsigned long long>(length) >> (8 * c)) % 256);
        }
        else
          out_header->put(static_cast<char>(length));

        LockGuard lock(send_queue_mutex);
        send_queue.emplace_back(out_header, out_message, callback);
        if(send_queue.size() == 1)
          send_from_queue();
      }

      /// Convenience function for sending a string.
      /// fin_rsv_opcode: 129=one fragment, text, 130=one fragment, binary, 136=close connection.
      /// See http://tools.ietf.org/html/rfc6455#section-5.2 for more information.
      void send(string_view out_message_str, const std::function<void(const error_code &)> &callback = nullptr, unsigned char fin_rsv_opcode = 129) {
        auto out_message = std::make_shared<OutMessage>();
        out_message->write(out_message_str.data(), static_cast<std::streamsize>(out_message_str.size()));
        send(out_message, callback, fin_rsv_opcode);
      }

      void send_close(int status, const std::string &reason = "", const std::function<void(const error_code &)> &callback = nullptr) {
        // Send close only once (in case close is initiated by server)
        if(closed)
          return;
        closed = true;

        auto send_stream = std::make_shared<OutMessage>();

        send_stream->put(status >> 8);
        send_stream->put(status % 256);

        *send_stream << reason;

        // fin_rsv_opcode=136: message close
        send(send_stream, callback, 136);
      }
    };

    class Endpoint {
      friend class SocketServerBase<socket_type>;

    private:
      Mutex connections_mutex;
      std::unordered_set<std::shared_ptr<Connection>> connections GUARDED_BY(connections_mutex);

    public:
      std::function<StatusCode(std::shared_ptr<Connection>, CaseInsensitiveMultimap &)> on_handshake;
      std::function<void(std::shared_ptr<Connection>)> on_open;
      std::function<void(std::shared_ptr<Connection>, std::shared_ptr<InMessage>)> on_message;
      std::function<void(std::shared_ptr<Connection>, int, const std::string &)> on_close;
      std::function<void(std::shared_ptr<Connection>, const error_code &)> on_error;
      std::function<void(std::shared_ptr<Connection>)> on_ping;
      std::function<void(std::shared_ptr<Connection>)> on_pong;

      std::unordered_set<std::shared_ptr<Connection>> get_connections() noexcept {
        LockGuard lock(connections_mutex);
        auto copy = connections;
        return copy;
      }
    };

    class Config {
      friend class SocketServerBase<socket_type>;

    private:
      Config(unsigned short port) noexcept : port(port) {}

    public:
      /// Port number to use. Defaults to 80 for HTTP and 443 for HTTPS. Set to 0 get an assigned port.
      unsigned short port;
      /// If io_service is not set, number of threads that the server will use when start() is called.
      /// Defaults to 1 thread.
      std::size_t thread_pool_size = 1;
      /// Timeout on request handling. Defaults to 5 seconds.
      long timeout_request = 5;
      /// Idle timeout. Defaults to no timeout.
      long timeout_idle = 0;
      /// Maximum size of incoming messages. Defaults to architecture maximum.
      /// Exceeding this limit will result in a message_size error code and the connection will be closed.
      std::size_t max_message_size = std::numeric_limits<std::size_t>::max();
      /// Additional header fields to send when performing WebSocket handshake.
      CaseInsensitiveMultimap header;
      /// IPv4 address in dotted decimal form or IPv6 address in hexadecimal notation.
      /// If empty, the address will be any address.
      std::string address;
      /// Set to false to avoid binding the socket to an address that is already in use. Defaults to true.
      bool reuse_address = true;
    };
    /// Set before calling start().
    Config config;

  private:
    class regex_orderable : public regex::regex {
    public:
      std::string str;

      regex_orderable(const char *regex_cstr) : regex::regex(regex_cstr), str(regex_cstr) {}
      regex_orderable(const std::string &regex_str) : regex::regex(regex_str), str(regex_str) {}
      bool operator<(const regex_orderable &rhs) const noexcept {
        return str < rhs.str;
      }
    };

  public:
    /// Warning: do not add or remove endpoints after start() is called
    std::map<regex_orderable, Endpoint> endpoint;

    /// If you know the server port in advance, use start() instead.
    /// Returns assigned port. If io_service is not set, an internal io_service is created instead.
    /// Call before accept_and_run().
    unsigned short bind() {
      asio::ip::tcp::endpoint endpoint;
      if(config.address.size() > 0)
        endpoint = asio::ip::tcp::endpoint(make_address(config.address), config.port);
      else
        endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v6(), config.port);

      if(!io_service) {
        io_service = std::make_shared<io_context>();
        internal_io_service = true;
      }

      if(!acceptor)
        acceptor = std::unique_ptr<asio::ip::tcp::acceptor>(new asio::ip::tcp::acceptor(*io_service));
      acceptor->open(endpoint.protocol());
      acceptor->set_option(asio::socket_base::reuse_address(config.reuse_address));
      acceptor->bind(endpoint);

      after_bind();

      return acceptor->local_endpoint().port();
    }

    /// If you know the server port in advance, use start() instead.
    /// Accept requests, and if io_service was not set before calling bind(), run the internal io_service instead.
    /// Call after bind().
    void accept_and_run() {
      acceptor->listen();
      accept();

      if(internal_io_service) {
        if(io_service->stopped())
          restart(*io_service);

        // If thread_pool_size>1, start m_io_service.run() in (thread_pool_size-1) threads for thread-pooling
        threads.clear();
        for(std::size_t c = 1; c < config.thread_pool_size; c++) {
          threads.emplace_back([this]() {
            this->io_service->run();
          });
        }

        // Main thread
        if(config.thread_pool_size > 0)
          io_service->run();

        // Wait for the rest of the threads, if any, to finish as well
        for(auto &t : threads)
          t.join();
      }
    }

    /// Start the server by calling bind() and accept_and_run()
    void start() {
      bind();
      accept_and_run();
    }

    /// Stop accepting new connections, and close current connections
    void stop() noexcept {
      if(acceptor) {
        error_code ec;
        acceptor->close(ec);

        for(auto &pair : endpoint) {
          LockGuard lock(pair.second.connections_mutex);
          for(auto &connection : pair.second.connections)
            connection->close();
          pair.second.connections.clear();
        }

        if(internal_io_service)
          io_service->stop();
      }
    }

    /// Stop accepting new connections
    void stop_accept() noexcept {
      if(acceptor) {
        error_code ec;
        acceptor->close(ec);
      }
    }

    virtual ~SocketServerBase() noexcept {}

    std::unordered_set<std::shared_ptr<Connection>> get_connections() noexcept {
      std::unordered_set<std::shared_ptr<Connection>> all_connections;
      for(auto &e : endpoint) {
        LockGuard lock(e.second.connections_mutex);
        all_connections.insert(e.second.connections.begin(), e.second.connections.end());
      }
      return all_connections;
    }

    /**
     * Upgrades a request, from for instance Simple-Web-Server, to a WebSocket connection.
     * The parameters are moved to the Connection object.
     * See also Server::on_upgrade in the Simple-Web-Server project.
     * The socket's io_service is used, thus running start() is not needed.
     *
     * Example use:
     * server.on_upgrade=[&socket_server] (auto socket, auto request) {
     *   auto connection=std::make_shared<SimpleWeb::SocketServer<SimpleWeb::WS>::Connection>(std::move(socket));
     *   connection->method=std::move(request->method);
     *   connection->path=std::move(request->path);
     *   connection->query_string=std::move(request->query_string);
     *   connection->http_version=std::move(request->http_version);
     *   connection->header=std::move(request->header);
     *   socket_server.upgrade(connection);
     * }
     */
    void upgrade(const std::shared_ptr<Connection> &connection) {
      connection->handler_runner = handler_runner;
      connection->timeout_idle = config.timeout_idle;
      write_handshake(connection);
    }

    /// If you have your own io_context, store its pointer here before running start().
    std::shared_ptr<io_context> io_service;

  protected:
    bool internal_io_service = false;

    std::unique_ptr<asio::ip::tcp::acceptor> acceptor;
    std::vector<std::thread> threads;

    std::shared_ptr<ScopeRunner> handler_runner;

    SocketServerBase(unsigned short port) noexcept : config(port), handler_runner(new ScopeRunner()) {}

    virtual void after_bind() {}
    virtual void accept() = 0;

    void read_handshake(const std::shared_ptr<Connection> &connection) {
      connection->set_timeout(config.timeout_request);
      asio::async_read_until(*connection->socket, connection->read_buffer, "\r\n\r\n", [this, connection](const error_code &ec, std::size_t /*bytes_transferred*/) {
        connection->cancel_timeout();
        auto lock = connection->handler_runner->continue_lock();
        if(!lock)
          return;
        if(!ec) {
          std::istream stream(&connection->read_buffer);
          if(RequestMessage::parse(stream, connection->method, connection->path, connection->query_string, connection->http_version, connection->header))
            write_handshake(connection);
        }
      });
    }

    void write_handshake(const std::shared_ptr<Connection> &connection) {
      for(auto &regex_endpoint : endpoint) {
        regex::smatch path_match;
        if(regex::regex_match(connection->path, path_match, regex_endpoint.first)) {
          auto write_buffer = std::make_shared<asio::streambuf>();
          std::ostream handshake(write_buffer.get());

          StatusCode status_code = StatusCode::information_switching_protocols;
          auto key_it = connection->header.find("Sec-WebSocket-Key");
          if(key_it == connection->header.end())
            status_code = StatusCode::client_error_upgrade_required;
          else {
            CaseInsensitiveMultimap response_header = config.header;
            response_header.emplace("Upgrade", "websocket");
            response_header.emplace("Connection", "Upgrade");
            static auto ws_magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            auto sha1 = Crypto::sha1(key_it->second + ws_magic_string);
            response_header.emplace("Sec-WebSocket-Accept", Crypto::Base64::encode(sha1));

            if(regex_endpoint.second.on_handshake)
              status_code = regex_endpoint.second.on_handshake(connection, response_header);

            if(status_code == StatusCode::information_switching_protocols) {
              handshake << "HTTP/1.1 101 Web Socket Protocol Handshake\r\n";
              for(auto &header_field : response_header)
                handshake << header_field.first << ": " << header_field.second << "\r\n";
              handshake << "\r\n";
            }
          }
          if(status_code != StatusCode::information_switching_protocols)
            handshake << "HTTP/1.1 " + SimpleWeb::status_code(status_code) + "\r\n\r\n";

          connection->path_match = std::move(path_match);
          connection->set_timeout(config.timeout_request);
          asio::async_write(*connection->socket, *write_buffer, [this, connection, write_buffer, &regex_endpoint, status_code](const error_code &ec, std::size_t /*bytes_transferred*/) {
            connection->cancel_timeout();
            auto lock = connection->handler_runner->continue_lock();
            if(!lock)
              return;
            if(status_code != StatusCode::information_switching_protocols)
              return;
            if(!ec) {
              connection_open(connection, regex_endpoint.second);
              read_message(connection, regex_endpoint.second);
            }
            else
              connection_error(connection, regex_endpoint.second, ec);
          });
          return;
        }
      }
    }

    void read_message(const std::shared_ptr<Connection> &connection, Endpoint &endpoint) const {
      asio::async_read(*connection->socket, connection->read_buffer, asio::transfer_exactly(2), [this, connection, &endpoint](const error_code &ec, std::size_t bytes_transferred) {
        auto lock = connection->handler_runner->continue_lock();
        if(!lock)
          return;
        if(!ec) {
          if(bytes_transferred == 0) { // TODO: why does this happen sometimes?
            read_message(connection, endpoint);
            return;
          }
          std::istream stream(&connection->read_buffer);

          std::array<unsigned char, 2> first_bytes;
          stream.read((char *)&first_bytes[0], 2);

          unsigned char fin_rsv_opcode = first_bytes[0];

          // Close connection if unmasked message from client (protocol error)
          if(first_bytes[1] < 128) {
            const std::string reason("message from client not masked");
            connection->send_close(1002, reason);
            connection_close(connection, endpoint, 1002, reason);
            return;
          }

          std::size_t length = (first_bytes[1] & 127);

          if(length == 126) {
            // 2 next bytes is the size of content
            asio::async_read(*connection->socket, connection->read_buffer, asio::transfer_exactly(2), [this, connection, &endpoint, fin_rsv_opcode](const error_code &ec, std::size_t /*bytes_transferred*/) {
              auto lock = connection->handler_runner->continue_lock();
              if(!lock)
                return;
              if(!ec) {
                std::istream stream(&connection->read_buffer);

                std::array<unsigned char, 2> length_bytes;
                stream.read((char *)&length_bytes[0], 2);

                std::size_t length = 0;
                std::size_t num_bytes = 2;
                for(std::size_t c = 0; c < num_bytes; c++)
                  length += static_cast<std::size_t>(length_bytes[c]) << (8 * (num_bytes - 1 - c));

                read_message_content(connection, length, endpoint, fin_rsv_opcode);
              }
              else
                connection_error(connection, endpoint, ec);
            });
          }
          else if(length == 127) {
            // 8 next bytes is the size of content
            asio::async_read(*connection->socket, connection->read_buffer, asio::transfer_exactly(8), [this, connection, &endpoint, fin_rsv_opcode](const error_code &ec, std::size_t /*bytes_transferred*/) {
              auto lock = connection->handler_runner->continue_lock();
              if(!lock)
                return;
              if(!ec) {
                std::istream stream(&connection->read_buffer);

                std::array<unsigned char, 8> length_bytes;
                stream.read((char *)&length_bytes[0], 8);

                std::size_t length = 0;
                std::size_t num_bytes = 8;
                for(std::size_t c = 0; c < num_bytes; c++)
                  length += static_cast<std::size_t>(length_bytes[c]) << (8 * (num_bytes - 1 - c));

                read_message_content(connection, length, endpoint, fin_rsv_opcode);
              }
              else
                connection_error(connection, endpoint, ec);
            });
          }
          else
            read_message_content(connection, length, endpoint, fin_rsv_opcode);
        }
        else
          connection_error(connection, endpoint, ec);
      });
    }

    void read_message_content(const std::shared_ptr<Connection> &connection, std::size_t length, Endpoint &endpoint, unsigned char fin_rsv_opcode) const {
      if(length + (connection->fragmented_in_message ? connection->fragmented_in_message->length : 0) > config.max_message_size) {
        connection_error(connection, endpoint, make_error_code::make_error_code(errc::message_size));
        const int status = 1009;
        const std::string reason = "message too big";
        connection->send_close(status, reason);
        connection_close(connection, endpoint, status, reason);
        return;
      }
      asio::async_read(*connection->socket, connection->read_buffer, asio::transfer_exactly(4 + length), [this, connection, length, &endpoint, fin_rsv_opcode](const error_code &ec, std::size_t /*bytes_transferred*/) {
        auto lock = connection->handler_runner->continue_lock();
        if(!lock)
          return;
        if(!ec) {
          std::istream istream(&connection->read_buffer);

          // Read mask
          std::array<unsigned char, 4> mask;
          istream.read((char *)&mask[0], 4);

          std::shared_ptr<InMessage> in_message;

          // If fragmented message
          if((fin_rsv_opcode & 0x80) == 0 || (fin_rsv_opcode & 0x0f) == 0) {
            if(!connection->fragmented_in_message) {
              connection->fragmented_in_message = std::shared_ptr<InMessage>(new InMessage(fin_rsv_opcode, length));
              connection->fragmented_in_message->fin_rsv_opcode |= 0x80;
            }
            else
              connection->fragmented_in_message->length += length;
            in_message = connection->fragmented_in_message;
          }
          else
            in_message = std::shared_ptr<InMessage>(new InMessage(fin_rsv_opcode, length));
          std::ostream ostream(&in_message->streambuf);
          for(std::size_t c = 0; c < length; c++)
            ostream.put(istream.get() ^ mask[c % 4]);

          // If connection close
          if((fin_rsv_opcode & 0x0f) == 8) {
            connection->cancel_timeout();
            connection->set_timeout();

            int status = 0;
            if(length >= 2) {
              unsigned char byte1 = in_message->get();
              unsigned char byte2 = in_message->get();
              status = (static_cast<int>(byte1) << 8) + byte2;
            }

            auto reason = in_message->string();
            connection->send_close(status, reason);
            this->connection_close(connection, endpoint, status, reason);
          }
          // If ping
          else if((fin_rsv_opcode & 0x0f) == 9) {
            connection->cancel_timeout();
            connection->set_timeout();

            // Send pong
            auto out_message = std::make_shared<OutMessage>();
            *out_message << in_message->string();
            connection->send(out_message, nullptr, fin_rsv_opcode + 1);

            if(endpoint.on_ping)
              endpoint.on_ping(connection);

            // Next message
            this->read_message(connection, endpoint);
          }
          // If pong
          else if((fin_rsv_opcode & 0x0f) == 10) {
            connection->cancel_timeout();
            connection->set_timeout();

            if(endpoint.on_pong)
              endpoint.on_pong(connection);

            // Next message
            this->read_message(connection, endpoint);
          }
          // If fragmented message and not final fragment
          else if((fin_rsv_opcode & 0x80) == 0) {
            // Next message
            this->read_message(connection, endpoint);
          }
          else {
            connection->cancel_timeout();
            connection->set_timeout();

            if(endpoint.on_message)
              endpoint.on_message(connection, in_message);

            // Next message
            // Only reset fragmented_in_message for non-control frames (control frames can be in between a fragmented message)
            connection->fragmented_in_message = nullptr;
            this->read_message(connection, endpoint);
          }
        }
        else
          this->connection_error(connection, endpoint, ec);
      });
    }

    void connection_open(const std::shared_ptr<Connection> &connection, Endpoint &endpoint) const {
      connection->cancel_timeout();
      connection->set_timeout();

      {
        LockGuard lock(endpoint.connections_mutex);
        endpoint.connections.insert(connection);
      }

      if(endpoint.on_open)
        endpoint.on_open(connection);
    }

    void connection_close(const std::shared_ptr<Connection> &connection, Endpoint &endpoint, int status, const std::string &reason) const {
      connection->cancel_timeout();
      connection->set_timeout();

      {
        LockGuard lock(endpoint.connections_mutex);
        endpoint.connections.erase(connection);
      }

      if(endpoint.on_close)
        endpoint.on_close(connection, status, reason);
    }

    void connection_error(const std::shared_ptr<Connection> &connection, Endpoint &endpoint, const error_code &ec) const {
      connection->cancel_timeout();
      connection->set_timeout();

      {
        LockGuard lock(endpoint.connections_mutex);
        endpoint.connections.erase(connection);
      }

      if(endpoint.on_error)
        endpoint.on_error(connection, ec);
    }
  };

  template <class socket_type>
  class SocketServer : public SocketServerBase<socket_type> {};

  using WS = asio::ip::tcp::socket;

  template <>
  class SocketServer<WS> : public SocketServerBase<WS> {
  public:
    SocketServer() noexcept : SocketServerBase<WS>(80) {}

  protected:
    void accept() override {
      std::shared_ptr<Connection> connection(new Connection(handler_runner, config.timeout_idle, *io_service));

      acceptor->async_accept(*connection->socket, [this, connection](const error_code &ec) {
        auto lock = connection->handler_runner->continue_lock();
        if(!lock)
          return;
        // Immediately start accepting a new connection (if io_service hasn't been stopped)
        if(ec != error::operation_aborted)
          accept();

        if(!ec) {
          asio::ip::tcp::no_delay option(true);
          connection->socket->set_option(option);

          read_handshake(connection);
        }
      });
    }
  };
} // namespace SimpleWeb

#endif /* SIMPLE_WEB_SERVER_WS_HPP */
