#pragma once
#include <QDialog>

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"
#include "PlotJuggler/messageparser_base.h"
#include "ui_datastream_zmq.h"
#include "zmq.hpp"

class StreamZMQDialog : public QDialog
{
  Q_OBJECT

public:
  explicit StreamZMQDialog(QWidget* parent = nullptr);
  ~StreamZMQDialog();

  Ui::DataStreamZMQ* ui;
};

class DataStreamZMQ : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  DataStreamZMQ();

  virtual ~DataStreamZMQ() override;

  virtual bool start(QStringList*) override;

  virtual void shutdown() override;

  virtual bool isRunning() const override
  {
    return _running;
  }

  virtual const char* name() const override
  {
    return "ZMQ Subscriber";
  }

  virtual bool isDebugPlugin() override
  {
    return false;
  }

private:
  bool _running;
  zmq::context_t _zmq_context;
  zmq::socket_t _zmq_socket;
  PJ::MessageParserPtr _parser;
  std::string _socket_address;
  std::thread _receive_thread;
  std::vector<std::string> _topic_filters;

  void receiveLoop();
  bool parseMessage(const PJ::MessageRef& msg, double& timestamp);
  void parseTopicFilters(const QString& filters);
  void subscribeTopics();
  void unsubscribeTopics();
};
