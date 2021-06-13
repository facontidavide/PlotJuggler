#ifndef DATA_STREAMER_TEMPLATE_H
#define DATA_STREAMER_TEMPLATE_H

#include <mutex>
#include <unordered_set>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/pj_plugin.h"
#include "PlotJuggler/messageparser_base.h"

namespace PJ {

/**
 * @brief The DataStreamer base class to create your own plugin.
 *
 * Important. To avoid problems with thread safety, it is important that ANY update to
 * dataMap(), which share its elements with the main application, is protected by the mutex()
 *
 * In particular the periodic updates.
 */
class DataStreamer : public PlotJugglerPlugin
{
  Q_OBJECT
public:

  DataStreamer(): _available_parsers(nullptr) {}

  virtual bool start(QStringList*) = 0;

  virtual void shutdown() = 0;

  virtual bool isRunning() const = 0;

  virtual ~DataStreamer() = default;

  std::mutex& mutex()
  {
    return _mutex;
  }

  void setMaximumRangeX(double range);

  PlotDataMapRef& dataMap()
  {
    return _data_map;
  }

  const PlotDataMapRef& dataMap() const
  {
    return _data_map;
  }

  void setAvailableParsers(MessageParserFactory* parsers )
  {
    _available_parsers = parsers;
  }

  MessageParserFactory* availableParsers()
  {
    if( _available_parsers && _available_parsers->empty() )
    {
      return nullptr;
    }
    return _available_parsers;
  }

signals:

  // Request to clear previous data
  void clearBuffers();

  // Remove a list of timeseries
  void removeGroup(std::string group_name);

  // signal published periodically when there is new data
  void dataReceived();

  // Stopping a plugin from the "inside"
  void closed();

private:
  std::mutex _mutex;
  PlotDataMapRef _data_map;
  QAction* _start_streamer;
  MessageParserFactory* _available_parsers;
};

inline void DataStreamer::setMaximumRangeX(double range)
{
  std::lock_guard<std::mutex> lock(mutex());
  for (auto& it : dataMap().numeric)
  {
    it.second.setMaximumRangeX(range);
  }
  for (auto& it : dataMap().strings)
  {
    it.second.setMaximumRangeX(range);
  }
  for (auto& it : dataMap().user_defined)
  {
    it.second.setMaximumRangeX(range);
  }
}

using DataStreamerPtr = std::shared_ptr<DataStreamer>;

}


QT_BEGIN_NAMESPACE
#define DataStream_iid "facontidavide.PlotJuggler3.DataStreamer"
Q_DECLARE_INTERFACE(PJ::DataStreamer, DataStream_iid)
QT_END_NAMESPACE

#endif
