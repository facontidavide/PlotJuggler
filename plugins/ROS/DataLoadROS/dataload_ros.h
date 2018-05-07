#ifndef DATALOAD_ROS_H
#define DATALOAD_ROS_H

#include <QObject>
#include <QtPlugin>

#include <ros/ros.h>
#include <rosbag/bag.h>

#include "PlotJuggler/dataloader_base.h"
#include <ros_type_introspection/ros_introspection.hpp>

class  DataLoadROS: public QObject, DataLoader
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.icarustechnology.PlotJuggler.DataLoader" "../dataloader.json")
    Q_INTERFACES(DataLoader)

public:
    DataLoadROS();
    virtual const std::vector<const char*>& compatibleFileExtensions() const override;

    virtual PlotDataMap readDataFromFile(const QString& file_name, bool use_previous_configuration ) override;

    virtual const char* name() const override { return "DataLoad ROS bags"; }

    virtual ~DataLoadROS();

    virtual QDomElement xmlSaveState(QDomDocument &doc) const override;

    virtual bool xmlLoadState(QDomElement &parent_element ) override;

protected:
    void loadSubstitutionRule(QStringList all_topic_names);
    std::shared_ptr<rosbag::Bag> _bag;

private:
    RosIntrospection::SubstitutionRuleMap  _rules;

    std::vector<const char*> _extensions;

    QStringList _default_topic_names;

    std::unique_ptr<RosIntrospection::Parser> _parser;

    bool _use_renaming_rules;

};

#endif // DATALOAD_CSV_H
