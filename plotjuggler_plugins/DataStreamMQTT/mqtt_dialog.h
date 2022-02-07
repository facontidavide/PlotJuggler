#ifndef MQTT_DIALOG_H
#define MQTT_DIALOG_H

#include "ui_datastream_mqtt.h"
#include "mqtt_client.h"
#include <QSettings>
#include <QMessageBox>
#include <QUuid>
#include <QIntValidator>
#include <QTimer>

class MQTT_Dialog: public QDialog
{
public:
  MQTT_Dialog(MQTTClient::Ptr mosq_client);

  ~MQTT_Dialog();

  Ui::DataStreamMQTT* ui;

  void saveSettings();

public slots:

  void onButtonConnect();

  void onUpdateTopicList();

  void onSelectionChanged();

private:

  MQTTClient::Ptr _client;

  QTimer *_topic_list_timer;

  void changeConnectionState(bool connected);

  void onConnectionClosed();

private slots:

  void onLoadServerCertificate();

  void onLoadClientCertificate();

  void onLoadPrivateKey();

  QString _server_certificate_file;
  QString _client_certificate_file;
  QString _private_key_file;

  std::unordered_set<std::string> _topic_list;
};

#endif // MQTT_DIALOG_H
