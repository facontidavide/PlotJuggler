#ifndef DATASTREAM_MQTT_H
#define DATASTREAM_MQTT_H

#include <QDialog>
#include <QtPlugin>
#include <QTimer>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"
#include "PlotJuggler/messageparser_base.h"

#include "MQTTAsync.h"

using namespace PJ;

class DataStreamMQTT : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  DataStreamMQTT();

  virtual bool start(QStringList*) override;

  virtual void shutdown() override;

  virtual bool isRunning() const override;

  virtual ~DataStreamMQTT() override;

  virtual const char* name() const override
  {
    return "MQTT Subscriber";
  }

  virtual bool isDebugPlugin() override
  {
    return false;
  }

  bool _disconnection_done;
  bool _subscribed;
  bool _finished;
  MQTTAsync _client;

  QString _error_msg;

  QString _protocol;
  QString _topic_filter;
  int _qos;
  std::unordered_map<std::string, PJ::MessageParserPtr> _parsers;

  bool _protocol_issue;
  QTimer _protocol_issue_timer;

private:
  bool _running;

private slots:

};


#endif // DATASTREAM_MQTT_H
