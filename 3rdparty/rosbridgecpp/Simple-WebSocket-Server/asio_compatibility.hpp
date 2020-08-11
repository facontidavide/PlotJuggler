#ifndef SIMPLE_WEB_ASIO_COMPATIBILITY_HPP
#define SIMPLE_WEB_ASIO_COMPATIBILITY_HPP

#include <memory>

#ifdef USE_STANDALONE_ASIO
#include <asio.hpp>
#include <asio/steady_timer.hpp>
namespace SimpleWeb {
  namespace error = asio::error;
  using error_code = std::error_code;
  using errc = std::errc;
  using system_error = std::system_error;
  namespace make_error_code = std;
} // namespace SimpleWeb
#else
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
namespace SimpleWeb {
  namespace asio = boost::asio;
  namespace error = asio::error;
  using error_code = boost::system::error_code;
  namespace errc = boost::system::errc;
  using system_error = boost::system::system_error;
  namespace make_error_code = boost::system::errc;
} // namespace SimpleWeb
#endif

namespace SimpleWeb {
#if(USE_STANDALONE_ASIO && ASIO_VERSION >= 101300) || BOOST_ASIO_VERSION >= 101300
  using io_context = asio::io_context;
  using resolver_results = asio::ip::tcp::resolver::results_type;
  using async_connect_endpoint = asio::ip::tcp::endpoint;

  inline void restart(io_context &context) noexcept {
    context.restart();
  }
  inline asio::ip::address make_address(const std::string &str) noexcept {
    return asio::ip::make_address(str);
  }
  template <typename socket_type>
  asio::executor get_socket_executor(socket_type &socket) {
    return socket.get_executor();
  }
  template <typename handler_type>
  void async_resolve(asio::ip::tcp::resolver &resolver, const std::pair<std::string, std::string> &host_port, handler_type &&handler) {
    resolver.async_resolve(host_port.first, host_port.second, std::forward<handler_type>(handler));
  }
#else
  using io_context = asio::io_service;
  using resolver_results = asio::ip::tcp::resolver::iterator;
  using async_connect_endpoint = asio::ip::tcp::resolver::iterator;

  inline void restart(io_context &context) noexcept {
    context.reset();
  }
  inline asio::ip::address make_address(const std::string &str) noexcept {
    return asio::ip::address::from_string(str);
  }
  template <typename socket_type>
  io_context &get_socket_executor(socket_type &socket) {
    return socket.get_io_service();
  }
  template <typename handler_type>
  void async_resolve(asio::ip::tcp::resolver &resolver, const std::pair<std::string, std::string> &host_port, handler_type &&handler) {
    resolver.async_resolve(asio::ip::tcp::resolver::query(host_port.first, host_port.second), std::forward<handler_type>(handler));
  }
#endif
} // namespace SimpleWeb

#endif /* SIMPLE_WEB_ASIO_COMPATIBILITY_HPP */
