#ifndef SIMPLE_WEB_SERVER_WSS_HPP
#define SIMPLE_WEB_SERVER_WSS_HPP

#include "server_ws.hpp"
#include <algorithm>
#include <openssl/ssl.h>

#ifdef USE_STANDALONE_ASIO
#include <asio/ssl.hpp>
#else
#include <boost/asio/ssl.hpp>
#endif


namespace SimpleWeb {
  using WSS = asio::ssl::stream<asio::ip::tcp::socket>;

  template <>
  class SocketServer<WSS> : public SocketServerBase<WSS> {
    bool set_session_id_context = false;

  public:
    SocketServer(const std::string &cert_file, const std::string &private_key_file, const std::string &verify_file = std::string())
        : SocketServerBase<WSS>(443), context(asio::ssl::context::tlsv12) {
      context.use_certificate_chain_file(cert_file);
      context.use_private_key_file(private_key_file, asio::ssl::context::pem);

      if(verify_file.size() > 0) {
        context.load_verify_file(verify_file);
        context.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert |
                                asio::ssl::verify_client_once);
        set_session_id_context = true;
      }
    }

  protected:
    asio::ssl::context context;

    void after_bind() override {
      if(set_session_id_context) {
        // Creating session_id_context from address:port but reversed due to small SSL_MAX_SSL_SESSION_ID_LENGTH
        auto session_id_context = std::to_string(acceptor->local_endpoint().port()) + ':';
        session_id_context.append(config.address.rbegin(), config.address.rend());
        SSL_CTX_set_session_id_context(context.native_handle(), reinterpret_cast<const unsigned char *>(session_id_context.data()),
                                       std::min<std::size_t>(session_id_context.size(), SSL_MAX_SSL_SESSION_ID_LENGTH));
      }
    }

    void accept() override {
      std::shared_ptr<Connection> connection(new Connection(handler_runner, config.timeout_idle, *io_service, context));

      acceptor->async_accept(connection->socket->lowest_layer(), [this, connection](const error_code &ec) {
        auto lock = connection->handler_runner->continue_lock();
        if(!lock)
          return;
        // Immediately start accepting a new connection (if io_service hasn't been stopped)
        if(ec != error::operation_aborted)
          accept();

        if(!ec) {
          asio::ip::tcp::no_delay option(true);
          connection->socket->lowest_layer().set_option(option);

          connection->set_timeout(config.timeout_request);
          connection->socket->async_handshake(asio::ssl::stream_base::server, [this, connection](const error_code &ec) {
            auto lock = connection->handler_runner->continue_lock();
            if(!lock)
              return;
            connection->cancel_timeout();
            if(!ec)
              read_handshake(connection);
          });
        }
      });
    }
  };
} // namespace SimpleWeb

#endif /* SIMPLE_WEB_SERVER_WSS_HPP */
