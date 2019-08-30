#ifndef DATASTREAM_ROS_TOPIC_H
#define DATASTREAM_ROS_TOPIC_H

#include <QtPlugin>
#include <QAction>
#include <QTimer>
#include <thread>
#include <topic_tools/shape_shifter.h>
#include "PlotJuggler/datastreamer_base.h"
#include <ros_type_introspection/ros_introspection.hpp>
#include <rosgraph_msgs/Clock.h>
#include "../dialog_select_ros_topics.h"
#include "../RosMsgParsers/ros_parser.h"

class  DataStreamROS: public DataStreamer
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.icarustechnology.PlotJuggler.DataStreamer" "../datastreamer.json")
    Q_INTERFACES(DataStreamer)

public:

    DataStreamROS();

    virtual bool start(QStringList* selected_datasources) override;

    virtual bool start(QStringList* selected_datasources, const bool& fromCommandline) override;

    virtual void shutdown() override;

    virtual bool isRunning() const override;

    virtual ~DataStreamROS() override;

    virtual const char* name() const override { return "ROS Topic Streamer";  }

    virtual bool xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const override;

    virtual bool xmlLoadState(const QDomElement &parent_element ) override;

    virtual void addActionsToParentMenu( QMenu* menu ) override;

    virtual std::vector<QString> appendData(PlotDataMapRef& destination) override
    {
        _destination_data = &destination;
        return DataStreamer::appendData(destination);
    }

private:

    PlotDataMapRef* _destination_data;

    void topicCallback(const topic_tools::ShapeShifter::ConstPtr& msg, const std::string &topic_name);

    void clockCallback(const rosgraph_msgs::Clock::ConstPtr& msg);

    void extractInitialSamples();

    void timerCallback();

    void subscribe();

    void saveDefaultSettings();

    void loadDefaultSettings();

    bool _running;

    std::shared_ptr<ros::AsyncSpinner> _spinner;

    double _initial_time;

    std::string _prefix;

    ros::NodeHandlePtr _node;

    std::map<std::string, ros::Subscriber> _subscribers;

    RosIntrospection::SubstitutionRuleMap _rules;

    int _received_msg_count;

    QAction* _action_saveIntoRosbag;

    std::map<std::string, int> _msg_index;

    DialogSelectRosTopics::Configuration _config;

    RosMessageParser _ros_parser;

    QTimer* _periodic_timer;

    bool _roscore_disconnection_already_notified;

    double _prev_clock_time;

private:

    static void saveIntoRosbag(const PlotDataMapRef& data);
};

#endif // DATALOAD_CSV_H
