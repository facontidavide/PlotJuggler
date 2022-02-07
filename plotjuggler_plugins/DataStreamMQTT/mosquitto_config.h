#ifndef MOSQUITTO_CONFIG_H
#define MOSQUITTO_CONFIG_H

#include <mosquitto.h>
#include <string>
#include <vector>

struct MosquittoConfig {
  int protocol_version;
  int keepalive;
  std::string host;
  int port;
  int qos;
  bool retain;
  std::string bind_address;
#ifdef WITH_SRV
  bool use_srv;
#endif
  unsigned int max_inflight;
  std::string username;
  std::string password;

  std::string cafile;
  std::string capath;
  std::string certfile;
  std::string keyfile;
  std::string ciphers;
  bool insecure;
  std::string tls_version;
#  ifdef WITH_TLS_PSK
  std::string psk;
  std::string psk_identity;
#  endif

  bool clean_session; /* sub */
  std::vector<std::string> topics; /* sub */
  bool no_retain; /* sub */
  std::vector<std::string> filter_outs; /* sub */
  bool verbose; /* sub */
  bool eol; /* sub */
  int msg_count; /* sub */
#ifdef WITH_SOCKS
  std::string socks5_host;
  int socks5_port;
  std::string socks5_username;
  std::string socks5_password;
#endif
  mosquitto_property *connect_props;
  mosquitto_property *subscribe_props;
  mosquitto_property *unsubscribe_props;
  mosquitto_property *disconnect_props;
};

#endif // MOSQUITTO_CONFIG_H
