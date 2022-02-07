#ifndef DATASTREAM_MQTT_H
#define DATASTREAM_MQTT_H

#include <QDialog>
#include <QtPlugin>
#include <QTimer>
#include <QThread>
#include "PlotJuggler/datastreamer_base.h"
#include "PlotJuggler/messageparser_base.h"
#include "ui_datastream_mqtt.h"
#include "mqtt_dialog.h"

using namespace PJ;

class DataStreamMQTT : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  DataStreamMQTT();

  ~DataStreamMQTT() override;

  virtual bool start(QStringList*) override;

  virtual void shutdown() override;

  virtual bool isRunning() const override;

  virtual const char* name() const override
  {
    return "MQTT Subscriber (Mosquitto)";
  }

  virtual bool isDebugPlugin() override
  {
    return false;
  }

  std::pair<QAction*, int> notificationAction() override
  {
    return { _notification_action, _failed_parsing };
  }

  bool _running;

  std::unordered_map<std::string, PJ::MessageParserPtr> _parsers;

  MQTTClient::Ptr _mosq;

  std::thread _mqtt_thread;
  QString _protocol;

  QString _topic_to_parse;

  QAction* _notification_action;
  int _failed_parsing = 0;

  MQTT_Dialog* _dialog;
  std::shared_ptr<MessageParserCreator> _current_parser_creator;

private slots:

  void onComboProtocolChanged(const QString &);

  void onMessageReceived(const mosquitto_message* message);
};


#endif // DATASTREAM_MQTT_H
