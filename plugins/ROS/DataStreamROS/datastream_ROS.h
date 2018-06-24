#ifndef DATASTREAM_ROS_TOPIC_H
#define DATASTREAM_ROS_TOPIC_H

#include <QtPlugin>
#include <QAction>
#include <QTimer>
#include <thread>
#include <topic_tools/shape_shifter.h>
#include "PlotJuggler/datastreamer_base.h"
#include <ros_type_introspection/ros_introspection.hpp>


class  DataStreamROS: public DataStreamer
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.icarustechnology.PlotJuggler.DataStreamer" "../datastreamer.json")
    Q_INTERFACES(DataStreamer)

public:

    DataStreamROS();

    virtual PlotDataMap &getDataMap() override;

    virtual bool start() override;

    virtual void shutdown() override;

    virtual void enableStreaming(bool enable) override;

    virtual bool isStreamingRunning() const override;

    virtual ~DataStreamROS() override;

    virtual const char* name() const override { return "ROS Topic Streamer";  }

    virtual void setParentMenu(QMenu* menu) override;

    virtual QDomElement xmlSaveState(QDomDocument &doc) const override;

    virtual bool xmlLoadState(QDomElement &parent_element ) override;

private:

    void topicCallback(const topic_tools::ShapeShifter::ConstPtr& msg, const std::string &topic_name);

    PlotDataMap _plot_data;

    bool _enabled;
    bool _running;

    std::shared_ptr<ros::AsyncSpinner> _spinner;

    void extractInitialSamples();

    double _initial_time;
    bool _use_header_timestamp;
    int _max_array_size;
    std::string _prefix;

    ros::NodeHandlePtr _node;
    std::map<std::string, ros::Subscriber> _subscribers;
    RosIntrospection::SubstitutionRuleMap _rules;

    int _received_msg_count;

    QAction* _action_saveIntoRosbag;

    std::map<std::string, int> _msg_index;

    QStringList _default_topic_names;

    std::unique_ptr<RosIntrospection::Parser> _parser;
    bool _using_renaming_rules;

    QTimer* _periodic_timer;

    bool _roscore_disconnection_already_notified;

    void timerCallback();

    void subscribe();

private slots:

    void saveIntoRosbag();
};

#endif // DATALOAD_CSV_H
