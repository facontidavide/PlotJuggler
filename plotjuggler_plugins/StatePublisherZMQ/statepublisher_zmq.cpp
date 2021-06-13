#include "statepublisher_zmq.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <zmq.hpp>
#include <thread>

StatePublisherZMQ::StatePublisherZMQ()
  : _prev_dataplot(0), _thread(std::bind(&StatePublisherZMQ::run_thread, this)), _prev_time(0)
{
}

StatePublisherZMQ::~StatePublisherZMQ()
{
}

void StatePublisherZMQ::run_thread()
{
  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REP);
  socket.bind("tcp://*:6665");

  while (true)
  {
    zmq::message_t request;

    //  Wait for next request from client
    socket.recv(&request);
    const char* request_data = (const char*)request.data();

    if (strncmp(request_data, "[get_data_names]", 16) == 0)
    {
      QString string_reply;

      _mutex.lock();

      std::map<QString, double>::iterator it;
      for (it = _current_data.begin(); it != _current_data.end(); it++)
      {
        string_reply.append(it->first + QString(" "));
      }
      _mutex.unlock();

      zmq::message_t reply(string_reply.size());
      socket.send(reply);
    }
    else if (strncmp(request_data, "[get_data]", 10) == 0)
    {
      bool abort = false;
      QString string_request = QString::fromUtf8(&request_data[10], request.size() - 10);
      QStringList names = string_request.split(';');

      _mutex.lock();
      QString string_reply;

      for (int i = 0; i < names.count(); i++)
      {
        std::map<QString, double>::iterator it = _current_data.find(names.at(i));
        if (it == _current_data.end())
        {
          abort = true;
          break;
        }
        else
        {
          double value = it->second;
          string_reply.append(QString::number(value) + QString(" "));
        }
      }
      _mutex.unlock();

      if (abort)
      {
        zmq::message_t reply(5);
        memcpy(reply.data(), "Error", 5);
        socket.send(reply);
      }
      else
      {
        zmq::message_t reply(string_reply.size());
        socket.send(reply);
      }
    }
    else
    {
      zmq::message_t reply(5);
      memcpy(reply.data(), "Error", 5);
      socket.send(reply);
    }
  }
}

void StatePublisherZMQ::updateState(PlotDataMap* datamap, double current_time)
{
  if (datamap == 0)
  {
    _prev_dataplot = datamap;
    _prev_time = current_time;
    _mutex.lock();
    _current_data.clear();
    _mutex.unlock();
    return;
  }

  PlotDataMap::iterator it;

  _mutex.lock();
  if (datamap != _prev_dataplot || current_time != _prev_time)
  {
    for (it = datamap->begin(); it != datamap->end(); it++)
    {
      const QString& name = it->first;
      PlotDataPtr Timeseries = it->second;
      _current_data[name] = plotdata->getY(current_time);
    }
  }
  _mutex.unlock();

  _prev_dataplot = datamap;
  _prev_time = current_time;
}
