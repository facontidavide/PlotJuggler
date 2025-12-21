#pragma once

#include "PlotJuggler/messageparser_base.h"

#include <QCheckBox>
#include <QSettings>
#include <QDebug>
#include <string>

#include "ros_parser.h"

using namespace PJ;

class ParserFactoryROS2 : public ParserFactoryPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.ParserFactoryPlugin")
  Q_INTERFACES(PJ::ParserFactoryPlugin)

public:
  ParserFactoryROS2() = default;

  const char* name() const override
  {
    return "ParserFactoryROS2";
  }
  const char* encoding() const override
  {
    return "ros2msg";
  }

  MessageParserPtr createParser(const std::string& topic_name, const std::string& type_name,
                                const std::string& schema, PlotDataMapRef& data) override
  {
    std::string msg_type = QString::fromStdString(type_name).replace("/msg/", "/").toStdString();

    auto parser = std::make_shared<ParserROS>(topic_name, msg_type, schema,
                                              new RosMsgParser::ROS2_Deserializer(), data);
    QSettings settings;
    parser->enableTruncationCheck(settings.value("Preferences::truncation_check", true).toBool());
    return parser;
  }
};
