#ifndef DATASTREAM_ROS_TOPIC_H
#define DATASTREAM_ROS_TOPIC_H

#include <QtPlugin>
#include <QAction>
#include <thread>
#include <topic_tools/shape_shifter.h>
#include "PlotJuggler/datastreamer_base.h"
#include <ros_type_introspection/ros_introspection.hpp>


class  DataStreamROS: public QObject, DataStreamer
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

    virtual bool isStreamingEnabled() const override;

    virtual ~DataStreamROS() override;

    virtual const char* name() const override { return "ROS Topic Streamer";  }

    virtual QObject* getObject() override { return this; }

    virtual void setParentMenu(QMenu* menu) override;

private:

    void topicCallback(const topic_tools::ShapeShifter::ConstPtr& msg, const std::string &topic_name);

    void updateLoop();

    PlotDataMap _plot_data;

    bool _enabled;
    bool _running;

    std::thread _thread;

    std::map<std::string, RosIntrospection::ROSTypeList> _ros_type_map;

    void extractInitialSamples();

    double _initial_time;
    bool _use_header_timestamp;

    ros::NodeHandlePtr _node;
    std::vector<ros::Subscriber> _subscribers;
    RosIntrospection::SubstitutionRuleMap _rules;

    int _received_msg_count;

    QAction* _action_saveIntoRosbag;

private slots:

    void saveIntoRosbag();
};

#endif // DATALOAD_CSV_H
