#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "mosquitto_config.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <QObject>

class MQTTClient: public QObject
{
  Q_OBJECT

public:
  using Ptr = std::shared_ptr<MQTTClient>;

  MQTTClient();
  ~MQTTClient();

  bool connect(const MosquittoConfig& config);

  void disconnect();

  bool isConnected() const;

  using TopicCallback = std::function<void(const mosquitto_message *)>;
  void addMessageCallback(const std::string& topic, TopicCallback callback);

  bool _connected = false;

  void onMessageReceived(const mosquitto_message *message);

  const MosquittoConfig& config() const;

  std::unordered_set<std::string> getTopicList();

  void subscribe(const std::string& topic, int qos );

  void unsubscribe(const std::string& topic );

signals:

  void disconnected();

private:
  mosquitto *_mosq = nullptr;
  std::unordered_map<std::string, TopicCallback> _message_callbacks;
  std::unordered_set<std::string> _topics_set;
  std::mutex _mutex;
  MosquittoConfig _config;
};

#endif // MQTT_CLIENT_H
