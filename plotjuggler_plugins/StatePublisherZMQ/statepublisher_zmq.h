#ifndef STATE_PUBLISHER_ZMQ_H
#define STATE_PUBLISHER_ZMQ_H

#include <QObject>
#include <QtPlugin>
#include <zmq.hpp>
#include <thread>
#include <mutex>
#include "../statepublisher_base.h"

class StatePublisherZMQ : public QObject, StatePublisher
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.StatePublisher")
  Q_INTERFACES(PJ::StatePublisher)

public:
  StatePublisherZMQ();

  virtual void updateState(PlotDataMap* datamap, double current_time);

  virtual ~StatePublisherZMQ();

private:
  PlotDataMap* _prev_dataplot;

  void run_thread();

  std::thread _thread;
  std::mutex _mutex;

  std::map<QString, double> _current_data;
  double _prev_time;
};

#endif  // STATE_PUBLISHER_ZMQ_H
