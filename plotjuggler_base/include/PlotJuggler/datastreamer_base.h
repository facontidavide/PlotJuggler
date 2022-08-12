/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef DATA_STREAMER_TEMPLATE_H
#define DATA_STREAMER_TEMPLATE_H

#include <mutex>
#include <unordered_set>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/pj_plugin.h"
#include "PlotJuggler/messageparser_base.h"

namespace PJ
{
/**
 * @brief The DataStreamer base classm used to read streaming of data.
 *
 * Important. To avoid problems with thread safety, ANY update to
 * dataMap(), which share its elements with the main application, must be protected
 * using the mutex().
 */
class DataStreamer : public PlotJugglerPlugin
{
  Q_OBJECT
public:
  DataStreamer() = default;

  virtual ~DataStreamer() = default;

  /**
   * @brief start streaming.
   *
   * @param optional list of pre selected sources.
   * @return true if started correctly.
   */
  virtual bool start(QStringList* pre_selected_sources) = 0;

  /**
   * @brief shutdown Stop streaming
   */
  virtual void shutdown() = 0;

  /**
   * @brief isRunning
   *
   * @return true if start() called, false after shutwodn()
   */
  virtual bool isRunning() const = 0;

  /**
   * @brief Gets the action to execute when clicking the 'notifications' button and
   * the current number of outstanding notifications.
   */
  virtual std::pair<QAction*, int> notificationAction()
  {
    return { nullptr, 0 };
  }

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

  void setParserFactories( ParserFactories* parsers);

  const ParserFactories* parserFactories() const;

signals:

  /// Request the main application to clear previous data points
  void clearBuffers();

  /// All the series which share this group, will be deleted
  void removeGroup(std::string group_name);

  /// Signal to be published periodically to notify to the main app
  /// that new data is available.
  void dataReceived();

  /// The method shutdown () is used by the main app to stop streaming.
  /// When the plugin stops itself, this signal must be emitted.
  void closed();

  // Plugin notifications.
  // PJ modifies the "notifications" button to indicate whether there are any
  void notificationsChanged(int active_notification_count);

private:

  std::mutex _mutex;
  PlotDataMapRef _data_map;
  QAction* _start_streamer;
  ParserFactories* _parser_factories = nullptr;
};

using DataStreamerPtr = std::shared_ptr<DataStreamer>;

}  // namespace PJ

QT_BEGIN_NAMESPACE
#define DataStream_iid "facontidavide.PlotJuggler3.DataStreamer"
Q_DECLARE_INTERFACE(PJ::DataStreamer, DataStream_iid)
QT_END_NAMESPACE

#endif
