#include "mqtt_client.h"
#include <QDebug>
#include <QMessageBox>
#include <QString>

#ifdef WIN32
#include <windows.h>
#include <strsafe.h>
#endif

void connect_callback(struct mosquitto *mosq, void *context, int result,  int, const mosquitto_property *)
{
  MQTTClient* self = static_cast<MQTTClient*>(context);

  if( !result )
  {
    for(const auto& topic: self->config().topics)
    {
      mosquitto_subscribe(mosq, nullptr, topic.c_str(), self->config().qos);
    }
  }
  else
  {
    QMessageBox::warning(nullptr, "MQTT Client",
                         QString("Connection error: %1").arg(mosquitto_reason_string(result)),
                         QMessageBox::Ok);
  }
  self->_connected = true;
}

void disconnect_callback(struct mosquitto *mosq, void *context, int result)
{
  MQTTClient* self = static_cast<MQTTClient*>(context);

  if( self->isConnected() && result == MOSQ_ERR_CONN_LOST )
  {
    emit self->disconnected();
  }
}

void message_callback(struct mosquitto *mosq, void *context, const struct mosquitto_message *message, const mosquitto_property *)
{
  MQTTClient* self = static_cast<MQTTClient*>(context);
  self->onMessageReceived(message);
}

//----------------------------

MQTTClient::MQTTClient()
{
  mosquitto_lib_init();
  _mosq = mosquitto_new(nullptr, true, this);

  mosquitto_connect_v5_callback_set(_mosq, connect_callback);
  mosquitto_disconnect_callback_set(_mosq, disconnect_callback);
  mosquitto_message_v5_callback_set(_mosq, message_callback);
}

MQTTClient::~MQTTClient()
{
  if(_connected)
  {
    disconnect();
  }
  mosquitto_lib_cleanup();
}

bool MQTTClient::connect(const MosquittoConfig &config)
{
  if( _connected )
  {
    disconnect();
  }

  mosquitto_int_option(_mosq, MOSQ_OPT_PROTOCOL_VERSION, config.protocol_version);

  if(( !config.username.empty() || !config.password.empty()) )
  {
    if(mosquitto_username_pw_set(_mosq, config.username.c_str(), config.password.c_str()))
    {
      return false;
    }
  }

  if( config.cafile.empty() == false )
  {
    const char *cafile = config.cafile.c_str();
    const char *certfile = config.certfile.empty() ? nullptr: config.certfile.c_str();
    const char *keyfile = config.keyfile.empty() ? nullptr: config.keyfile.c_str();

    mosquitto_tls_set(_mosq, cafile, nullptr, certfile, keyfile, nullptr);
  }

  mosquitto_max_inflight_messages_set(_mosq, config.max_inflight);

  const mosquitto_property *properties = nullptr; // todo

  int rc = mosquitto_connect_bind_v5(_mosq,
                                     config.host.c_str(),
                                     config.port,
                                     config.keepalive,
                                     nullptr,
                                     properties);
  // TODO bind
  if(rc>0)
  {
    if(rc == MOSQ_ERR_ERRNO)
    {
      char err[1024];
#ifndef WIN32
      strerror_r(errno, err, 1024);
#else
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errno, 0, (LPTSTR)&err, 1024, NULL);
#endif
      QMessageBox::warning(nullptr, "MQTT Client",
                           QString("Error: %1").arg(err),  QMessageBox::Ok);
    }
    else
    {
      QMessageBox::warning(nullptr, "MQTT Client",
                           QString("Unable to connect (%1)").arg(mosquitto_strerror(rc)),
                           QMessageBox::Ok);
    }
    _connected = false;
    return false;
  }

  _connected = true;
  _config = config;
  mosquitto_loop_start(_mosq);
  return true;
}

void MQTTClient::disconnect()
{
  if( _connected )
  {
    mosquitto_disconnect(_mosq);
    mosquitto_loop_stop(_mosq, true);
  }
  _connected = false;
  _topics_set.clear();
  _message_callbacks.clear();
}

bool MQTTClient::isConnected() const
{
  return _connected;
}

void MQTTClient::addMessageCallback(const std::string &topic, MQTTClient::TopicCallback callback)
{
  std::unique_lock<std::mutex> lk(_mutex);
  _message_callbacks[topic] = callback;
}

void MQTTClient::onMessageReceived(const mosquitto_message * message)
{
  std::unique_lock<std::mutex> lk(_mutex);

  _topics_set.insert(message->topic);

  auto it = _message_callbacks.find(message->topic);
  if( it != _message_callbacks.end() )
  {
    it->second(message);
  }
}

const MosquittoConfig &MQTTClient::config() const
{
  return _config;
}

std::unordered_set<std::string> MQTTClient::getTopicList()
{
  std::unique_lock<std::mutex> lk(_mutex);
  return _topics_set;
}

void MQTTClient::subscribe(const std::string &topic, int qos)
{
  mosquitto_subscribe(_mosq, nullptr, topic.c_str(), qos);
}

void MQTTClient::unsubscribe(const std::string &topic)
{
  mosquitto_unsubscribe(_mosq, nullptr, topic.c_str());
}
