#ifndef SIMPLE_WEB_CLIENT_WS_HPP
#define SIMPLE_WEB_CLIENT_WS_HPP

#include "asio_compatibility.hpp"
#include "crypto.hpp"
#include "mutex.hpp"
#include "utility.hpp"
#include <array>
#include <atomic>
#include <iostream>
#include <limits>
#include <list>
#include <random>

namespace SimpleWeb {
  template <class socket_type>
  class SocketClient;

  template <class socket_type>
  class SocketClientBase {
  public:
    class InMessage : public std::istream {
      friend class SocketClientBase<socket_type>;
      friend class SocketClient<socket_type>;
      friend class Connection;

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

    /// The buffer is consumed during send operations.
    class OutMessage : public std::iostream {
      friend class SocketClientBase<socket_type>;

      asio::streambuf streambuf;

    public:
      OutMessage() noexcept : std::iostream(&streambuf) {}
      OutMessage(std::size_t capacity) noexcept : std::iostream(&streambuf) {
        streambuf.prepare(capacity);
      }

      /// Returns the size of the buffer
      std::size_t size() const noexcept {
        return streambuf.size();
      }
    };

    class Connection : public std::enable_shared_from_this<Connection> {
      friend class SocketClientBase<socket_type>;
      friend class SocketClient<socket_type>;

    public:
      std::string http_version, status_code;
      CaseInsensitiveMultimap header;

      asio::ip::tcp::endpoint remote_endpoint;

      std::string remote_endpoint_address() noexcept {
        try {
          return remote_endpoint.address().to_string();
        }
        catch(...) {
          return std::string();
        }
      }

      unsigned short remote_endpoint_port() noexcept {
        return remote_endpoint.port();
      }

    private:
      template <typename... Args>
      Connection(std::shared_ptr<ScopeRunner> handler_runner_, long timeout_idle, Args &&... args) noexcept
          : handler_runner(std::move(handler_runner_)), socket(new socket_type(std::forward<Args>(args)...)), timeout_idle(timeout_idle), closed(false) {}

      std::shared_ptr<ScopeRunner> handler_runner;

      std::unique_ptr<socket_type> socket; // Socket must be unique_ptr since asio::ssl::stream<asio::ip::tcp::socket> is not movable

      std::shared_ptr<InMessage> in_message;
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
        OutData(std::shared_ptr<OutMessage> out_message_, std::function<void(const error_code)> &&callback_) noexcept
            : out_message(std::move(out_message_)), callback(std::move(callback_)) {}
        std::shared_ptr<OutMessage> out_message;
        std::function<void(const error_code)> callback;
      };

      Mutex send_queue_mutex;
      std::list<OutData> send_queue GUARDED_BY(send_queue_mutex);

      void send_from_queue() REQUIRES(send_queue_mutex) {
        auto self = this->shared_from_this();
        asio::async_write(*self->socket, send_queue.begin()->out_message->streambuf, [self](const error_code &ec, std::size_t /*bytes_transferred*/) {
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

      void read_remote_endpoint() noexcept {
        try {
          remote_endpoint = socket->lowest_layer().remote_endpoint();
        }
        catch(const std::exception &e) {
          std::cerr << e.what() << std::endl;
        }
      }

    public:
      /// fin_rsv_opcode: 129=one fragment, text, 130=one fragment, binary, 136=close connection.
      /// See http://tools.ietf.org/html/rfc6455#section-5.2 for more information.
      void send(const std::shared_ptr<OutMessage> &out_message, const std::function<void(const error_code &)> &callback = nullptr, unsigned char fin_rsv_opcode = 129) {
        cancel_timeout();
        set_timeout();

        // Create mask
        std::array<unsigned char, 4> mask;
        std::uniform_int_distribution<unsigned short> dist(0, 255);
        std::random_device rd;
        for(std::size_t c = 0; c < 4; c++)
          mask[c] = static_cast<unsigned char>(dist(rd));

        std::size_t length = out_message->size();

        std::size_t max_additional_bytes = 14; // ws protocol adds at most 14 bytes
        auto out_header_and_message = std::make_shared<OutMessage>(length + max_additional_bytes);

        out_header_and_message->put(static_cast<char>(fin_rsv_opcode));
        // Masked (first length byte>=128)
        if(length >= 126) {
          std::size_t num_bytes;
          if(length > 0xffff) {
            num_bytes = 8;
            out_header_and_message->put(static_cast<char>(127 + 128));
          }
          else {
            num_bytes = 2;
            out_header_and_message->put(static_cast<char>(126 + 128));
          }

          for(std::size_t c = num_bytes - 1; c != static_cast<std::size_t>(-1); c--)
            out_header_and_message->put((static_cast<unsigned long long>(length) >> (8 * c)) % 256);
        }
        else
          out_header_and_message->put(static_cast<char>(length + 128));

        for(std::size_t c = 0; c < 4; c++)
          out_header_and_message->put(static_cast<char>(mask[c]));

        for(std::size_t c = 0; c < length; c++)
          out_header_and_message->put(out_message->get() ^ mask[c % 4]);

        LockGuard lock(send_queue_mutex);
        send_queue.emplace_back(out_header_and_message, callback);
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
        // Send close only once (in case close is initiated by client)
        if(closed)
          return;
        closed = true;

        auto out_message = std::make_shared<OutMessage>();

        out_message->put(status >> 8);
        out_message->put(status % 256);

        *out_message << reason;

        // fin_rsv_opcode=136: message close
        send(out_message, callback, 136);
      }
    };

    class Config {
      friend class SocketClientBase<socket_type>;

    private:
      Config() noexcept {}

    public:
      /// Timeout on request handling. Defaults to no timeout.
      long timeout_request = 0;
      /// Idle timeout. Defaults to no timeout.
      long timeout_idle = 0;
      /// Maximum size of incoming messages. Defaults to architecture maximum.
      /// Exceeding this limit will result in a message_size error code and the connection will be closed.
      std::size_t max_message_size = std::numeric_limits<std::size_t>::max();
      /// Additional header fields to send when performing WebSocket upgrade.
      /// Use this variable to for instance set Sec-WebSocket-Protocol.
      CaseInsensitiveMultimap header;
      /// Set proxy server (server:port)
      std::string proxy_server;
    };
    /// Set before calling start().
    Config config;

    std::function<void(std::shared_ptr<Connection>)> on_open;
    std::function<void(std::shared_ptr<Connection>, std::shared_ptr<InMessage>)> on_message;
    std::function<void(std::shared_ptr<Connection>, int, const std::string &)> on_close;
    std::function<void(std::shared_ptr<Connection>, const error_code &)> on_error;
    std::function<void(std::shared_ptr<Connection>)> on_ping;
    std::function<void(std::shared_ptr<Connection>)> on_pong;

    void start() {
      if(!io_service) {
        io_service = std::make_shared<io_context>();
        internal_io_service = true;
      }

      if(io_service->stopped())
        restart(*io_service);

      connect();

      if(internal_io_service)
        io_service->run();
    }

    void stop() noexcept {
      {
        LockGuard lock(connection_mutex);
        if(connection)
          connection->close();
      }

      if(internal_io_service)
        io_service->stop();
    }

    virtual ~SocketClientBase() noexcept {
      handler_runner->stop();
      stop();
    }

    /// If you have your own io_context, store its pointer here before running start().
    std::shared_ptr<io_context> io_service;

  protected:
    bool internal_io_service = false;

    std::string host;
    unsigned short port;
    unsigned short default_port;
    std::string path;

    Mutex connection_mutex;
    std::shared_ptr<Connection> connection GUARDED_BY(connection_mutex);

    std::shared_ptr<ScopeRunner> handler_runner;

    SocketClientBase(const std::string &host_port_path, unsigned short default_port) noexcept : default_port(default_port), handler_runner(new ScopeRunner()) {
      auto host_port_end = host_port_path.find('/');
      auto host_port = parse_host_port(host_port_path.substr(0, host_port_end), default_port);
      host = std::move(host_port.first);
      port = host_port.second;

      if(host_port_end != std::string::npos)
        path = host_port_path.substr(host_port_end);
      else
        path = "/";
    }

    std::pair<std::string, unsigned short> parse_host_port(const std::string &host_port, unsigned short default_port) const noexcept {
      std::pair<std::string, unsigned short> parsed_host_port;
      std::size_t host_end = host_port.find(':');
      if(host_end == std::string::npos) {
        parsed_host_port.first = host_port;
        parsed_host_port.second = default_port;
      }
      else {
        parsed_host_port.first = host_port.substr(0, host_end);
        parsed_host_port.second = static_cast<unsigned short>(stoul(host_port.substr(host_end + 1)));
      }
      return parsed_host_port;
    }

    virtual void connect() = 0;

    void upgrade(const std::shared_ptr<Connection> &connection) {
      connection->read_remote_endpoint();

      auto corrected_path = path;
      if(!config.proxy_server.empty() && std::is_same<socket_type, asio::ip::tcp::socket>::value)
        corrected_path = "http://" + host + ':' + std::to_string(port) + corrected_path;

      auto write_buffer = std::make_shared<asio::streambuf>();
      std::ostream ostream(write_buffer.get());
      ostream << "GET " << corrected_path << " HTTP/1.1\r\n";
      ostream << "Host: " << host;
      if(port != default_port)
        ostream << ':' << std::to_string(port);
      ostream << "\r\n";
      ostream << "Upgrade: websocket\r\n";
      ostream << "Connection: Upgrade\r\n";

      // Make random 16-byte nonce
      std::string nonce;
      nonce.reserve(16);
      std::uniform_int_distribution<unsigned short> dist(0, 255);
      std::random_device rd;
      for(std::size_t c = 0; c < 16; c++)
        nonce += static_cast<char>(dist(rd));

      auto nonce_base64 = std::make_shared<std::string>(Crypto::Base64::encode(nonce));
      ostream << "Sec-WebSocket-Key: " << *nonce_base64 << "\r\n";
      ostream << "Sec-WebSocket-Version: 13\r\n";
      for(auto &header_field : config.header)
        ostream << header_field.first << ": " << header_field.second << "\r\n";
      ostream << "\r\n";

      connection->in_message = std::shared_ptr<InMessage>(new InMessage());

      connection->set_timeout(config.timeout_request);
      asio::async_write(*connection->socket, *write_buffer, [this, connection, write_buffer, nonce_base64](const error_code &ec, std::size_t /*bytes_transferred*/) {
        connection->cancel_timeout();
        auto lock = connection->handler_runner->continue_lock();
        if(!lock)
          return;
        if(!ec) {
          connection->set_timeout(this->config.timeout_request);
          asio::async_read_until(*connection->socket, connection->in_message->streambuf, "\r\n\r\n", [this, connection, nonce_base64](const error_code &ec, std::size_t bytes_transferred) {
            connection->cancel_timeout();
            auto lock = connection->handler_runner->continue_lock();
            if(!lock)
              return;
            if(!ec) {
              // connection->in_message->streambuf.size() is not necessarily the same as bytes_transferred, from Boost-docs:
              // "After a successful async_read_until operation, the streambuf may contain additional data beyond the delimiter"
              // The chosen solution is to extract lines from the stream directly when parsing the header. What is left of the
              // streambuf (maybe some bytes of a message) is appended to in the next async_read-function
              std::size_t num_additional_bytes = connection->in_message->streambuf.size() - bytes_transferred;

              if(!ResponseMessage::parse(*connection->in_message, connection->http_version, connection->status_code, connection->header)) {
                this->connection_error(connection, make_error_code::make_error_code(errc::protocol_error));
                return;
              }
              if(connection->status_code.compare(0, 4, "101 ") != 0) {
                this->connection_error(connection, make_error_code::make_error_code(errc::permission_denied));
                return;
              }
              auto header_it = connection->header.find("Sec-WebSocket-Accept");
              static auto ws_magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
              if(header_it != connection->header.end() &&
                 Crypto::Base64::decode(header_it->second) == Crypto::sha1(*nonce_base64 + ws_magic_string)) {
                this->connection_open(connection);
                read_message(connection, num_additional_bytes);
              }
              else
                this->connection_error(connection, make_error_code::make_error_code(errc::protocol_error));
            }
            else
              this->connection_error(connection, ec);
          });
        }
        else
          this->connection_error(connection, ec);
      });
    }

    void read_message(const std::shared_ptr<Connection> &connection, std::size_t num_additional_bytes) {
      asio::async_read(*connection->socket, connection->in_message->streambuf, asio::transfer_exactly(num_additional_bytes > 2 ? 0 : 2 - num_additional_bytes), [this, connection](const error_code &ec, std::size_t bytes_transferred) {
        auto lock = connection->handler_runner->continue_lock();
        if(!lock)
          return;
        if(!ec) {
          if(bytes_transferred == 0 && connection->in_message->streambuf.size() == 0) { // TODO: This might happen on server at least, might also happen here
            this->read_message(connection, 0);
            return;
          }
          std::size_t num_additional_bytes = connection->in_message->streambuf.size() - bytes_transferred;

          std::array<unsigned char, 2> first_bytes;
          connection->in_message->read(reinterpret_cast<char *>(&first_bytes[0]), 2);

          connection->in_message->fin_rsv_opcode = first_bytes[0];

          // Close connection if masked message from server (protocol error)
          if(first_bytes[1] >= 128) {
            const std::string reason("message from server masked");
            connection->send_close(1002, reason);
            this->connection_close(connection, 1002, reason);
            return;
          }

          std::size_t length = (first_bytes[1] & 127);

          if(length == 126) {
            // 2 next bytes is the size of content
            asio::async_read(*connection->socket, connection->in_message->streambuf, asio::transfer_exactly(num_additional_bytes > 2 ? 0 : 2 - num_additional_bytes), [this, connection](const error_code &ec, std::size_t bytes_transferred) {
              auto lock = connection->handler_runner->continue_lock();
              if(!lock)
                return;
              if(!ec) {
                std::size_t num_additional_bytes = connection->in_message->streambuf.size() - bytes_transferred;

                std::array<unsigned char, 2> length_bytes;
                connection->in_message->read(reinterpret_cast<char *>(&length_bytes[0]), 2);

                std::size_t length = 0;
                std::size_t num_bytes = 2;
                for(std::size_t c = 0; c < num_bytes; c++)
                  length += static_cast<std::size_t>(length_bytes[c]) << (8 * (num_bytes - 1 - c));

                connection->in_message->length = length;
                this->read_message_content(connection, num_additional_bytes);
              }
              else
                this->connection_error(connection, ec);
            });
          }
          else if(length == 127) {
            // 8 next bytes is the size of content
            asio::async_read(*connection->socket, connection->in_message->streambuf, asio::transfer_exactly(num_additional_bytes > 8 ? 0 : 8 - num_additional_bytes), [this, connection](const error_code &ec, std::size_t bytes_transferred) {
              auto lock = connection->handler_runner->continue_lock();
              if(!lock)
                return;
              if(!ec) {
                std::size_t num_additional_bytes = connection->in_message->streambuf.size() - bytes_transferred;

                std::array<unsigned char, 8> length_bytes;
                connection->in_message->read(reinterpret_cast<char *>(&length_bytes[0]), 8);

                std::size_t length = 0;
                std::size_t num_bytes = 8;
                for(std::size_t c = 0; c < num_bytes; c++)
                  length += static_cast<std::size_t>(length_bytes[c]) << (8 * (num_bytes - 1 - c));

                connection->in_message->length = length;
                this->read_message_content(connection, num_additional_bytes);
              }
              else
                this->connection_error(connection, ec);
            });
          }
          else {
            connection->in_message->length = length;
            this->read_message_content(connection, num_additional_bytes);
          }
        }
        else
          this->connection_error(connection, ec);
      });
    }

    void read_message_content(const std::shared_ptr<Connection> &connection, std::size_t num_additional_bytes) {
      if(connection->in_message->length + (connection->fragmented_in_message ? connection->fragmented_in_message->length : 0) > config.max_message_size) {
        connection_error(connection, make_error_code::make_error_code(errc::message_size));
        const int status = 1009;
        const std::string reason = "message too big";
        connection->send_close(status, reason);
        connection_close(connection, status, reason);
        return;
      }
      asio::async_read(*connection->socket, connection->in_message->streambuf, asio::transfer_exactly(num_additional_bytes > connection->in_message->length ? 0 : connection->in_message->length - num_additional_bytes), [this, connection](const error_code &ec, std::size_t bytes_transferred) {
        auto lock = connection->handler_runner->continue_lock();
        if(!lock)
          return;
        if(!ec) {
          std::size_t num_additional_bytes = connection->in_message->streambuf.size() - bytes_transferred;
          std::shared_ptr<InMessage> next_in_message;
          if(num_additional_bytes > 0) { // Extract bytes that are not extra bytes in buffer (only happen when several messages are sent in upgrade response)
            next_in_message = connection->in_message;
            connection->in_message = std::shared_ptr<InMessage>(new InMessage(next_in_message->fin_rsv_opcode, next_in_message->length));
            std::ostream ostream(&connection->in_message->streambuf);
            for(std::size_t c = 0; c < next_in_message->length; ++c)
              ostream.put(next_in_message->get());
          }
          else
            next_in_message = std::shared_ptr<InMessage>(new InMessage());

          // If connection close
          if((connection->in_message->fin_rsv_opcode & 0x0f) == 8) {
            connection->cancel_timeout();
            connection->set_timeout();

            int status = 0;
            if(connection->in_message->length >= 2) {
              unsigned char byte1 = connection->in_message->get();
              unsigned char byte2 = connection->in_message->get();
              status = (static_cast<int>(byte1) << 8) + byte2;
            }

            auto reason = connection->in_message->string();
            connection->send_close(status, reason);
            this->connection_close(connection, status, reason);
          }
          // If ping
          else if((connection->in_message->fin_rsv_opcode & 0x0f) == 9) {
            connection->cancel_timeout();
            connection->set_timeout();

            // Send pong
            auto out_message = std::make_shared<OutMessage>();
            *out_message << connection->in_message->string();
            connection->send(out_message, nullptr, connection->in_message->fin_rsv_opcode + 1);

            if(this->on_ping)
              this->on_ping(connection);

            // Next message
            connection->in_message = next_in_message;
            this->read_message(connection, num_additional_bytes);
          }
          // If pong
          else if((connection->in_message->fin_rsv_opcode & 0x0f) == 10) {
            connection->cancel_timeout();
            connection->set_timeout();

            if(this->on_pong)
              this->on_pong(connection);

            // Next message
            connection->in_message = next_in_message;
            this->read_message(connection, num_additional_bytes);
          }
          // If fragmented message and not final fragment
          else if((connection->in_message->fin_rsv_opcode & 0x80) == 0) {
            if(!connection->fragmented_in_message) {
              connection->fragmented_in_message = connection->in_message;
              connection->fragmented_in_message->fin_rsv_opcode |= 0x80;
            }
            else {
              connection->fragmented_in_message->length += connection->in_message->length;
              std::ostream ostream(&connection->fragmented_in_message->streambuf);
              ostream << connection->in_message->rdbuf();
            }

            // Next message
            connection->in_message = next_in_message;
            this->read_message(connection, num_additional_bytes);
          }
          else {
            connection->cancel_timeout();
            connection->set_timeout();

            if(this->on_message) {
              if(connection->fragmented_in_message) {
                connection->fragmented_in_message->length += connection->in_message->length;
                std::ostream ostream(&connection->fragmented_in_message->streambuf);
                ostream << connection->in_message->rdbuf();

                this->on_message(connection, connection->fragmented_in_message);
              }
              else
                this->on_message(connection, connection->in_message);
            }

            // Next message
            connection->in_message = next_in_message;
            // Only reset fragmented_message for non-control frames (control frames can be in between a fragmented message)
            connection->fragmented_in_message = nullptr;
            this->read_message(connection, num_additional_bytes);
          }
        }
        else
          this->connection_error(connection, ec);
      });
    }

    void connection_open(const std::shared_ptr<Connection> &connection) const {
      connection->cancel_timeout();
      connection->set_timeout();

      if(on_open)
        on_open(connection);
    }

    void connection_close(const std::shared_ptr<Connection> &connection, int status, const std::string &reason) const {
      connection->cancel_timeout();
      connection->set_timeout();

      if(on_close)
        on_close(connection, status, reason);
    }

    void connection_error(const std::shared_ptr<Connection> &connection, const error_code &ec) const {
      connection->cancel_timeout();
      connection->set_timeout();

      if(on_error)
        on_error(connection, ec);
    }
  };

  template <class socket_type>
  class SocketClient : public SocketClientBase<socket_type> {};

  using WS = asio::ip::tcp::socket;

  template <>
  class SocketClient<WS> : public SocketClientBase<WS> {
  public:
    SocketClient(const std::string &server_port_path) noexcept : SocketClientBase<WS>::SocketClientBase(server_port_path, 80){};

  protected:
    void connect() override {
      LockGuard lock(connection_mutex);
      auto connection = this->connection = std::shared_ptr<Connection>(new Connection(handler_runner, config.timeout_idle, *io_service));
      lock.unlock();

      std::pair<std::string, std::string> host_port;
      if(config.proxy_server.empty())
        host_port = {host, std::to_string(port)};
      else {
        auto proxy_host_port = parse_host_port(config.proxy_server, 8080);
        host_port = {proxy_host_port.first, std::to_string(proxy_host_port.second)};
      }

      auto resolver = std::make_shared<asio::ip::tcp::resolver>(*io_service);
      connection->set_timeout(config.timeout_request);
      async_resolve(*resolver, host_port, [this, connection, resolver](const error_code &ec, resolver_results results) {
        connection->cancel_timeout();
        auto lock = connection->handler_runner->continue_lock();
        if(!lock)
          return;
        if(!ec) {
          connection->set_timeout(this->config.timeout_request);
          asio::async_connect(*connection->socket, results, [this, connection, resolver](const error_code &ec, async_connect_endpoint /*endpoint*/) {
            connection->cancel_timeout();
            auto lock = connection->handler_runner->continue_lock();
            if(!lock)
              return;
            if(!ec) {
              asio::ip::tcp::no_delay option(true);
              connection->socket->set_option(option);

              this->upgrade(connection);
            }
            else
              this->connection_error(connection, ec);
          });
        }
        else
          this->connection_error(connection, ec);
      });
    }
  };
} // namespace SimpleWeb

#endif /* SIMPLE_WEB_CLIENT_WS_HPP */
