#ifndef DATASTREAM_ROS_TOPIC_H
#define DATASTREAM_ROS_TOPIC_H

#include <QtPlugin>
#include <QAction>
#include <QTimer>
#include <QWebSocket>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"
#include "dialog_select_ros_topics.h"

class DataStreamROSWS : public DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "com.icarustechnology.PlotJuggler.DataStreamer"
                        "../datastreamer.json")
  Q_INTERFACES(DataStreamer)

public:
  DataStreamROSWS();

  virtual bool start(QStringList* selected_datasources) override;

  virtual void shutdown() override;

  virtual bool isRunning() const override;

  virtual ~DataStreamROSWS() override;

  virtual const char* name() const override
  {
    return "ROS Topic Subscriber WS";
  }

  virtual bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  virtual bool xmlLoadState(const QDomElement& parent_element) override;

//  virtual void addActionsToParentMenu(QMenu* menu) override;


private slots:
    void onConnected();
    void onDisconnected();
    void handleWsMessage(const QString &message);


private:
  void subscribe();

  void saveDefaultSettings();

  void loadDefaultSettings();

  bool _running;

  double _initial_time;

  std::string _prefix;

  QWebSocket _ws;

  std::vector<std::pair<QString, QString>> _all_topics;

  bool _fetched_topics;

//  QAction* _action_saveIntoRosbag;

  std::map<std::string, int> _msg_index;

  DialogSelectRosTopics::Configuration _config;

  double _prev_clock_time;

};

#endif  // DATASTREAM_ROS_TOPIC_H
