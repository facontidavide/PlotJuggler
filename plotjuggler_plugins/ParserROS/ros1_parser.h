#pragma once

#include "PlotJuggler/messageparser_base.h"

#include <QCheckBox>
#include <QDebug>
#include <QSettings>
#include <string>

#include "ros_parser.h"

using namespace PJ;

class ParserFactoryROS1 : public ParserFactoryPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.ParserFactoryPlugin")
  Q_INTERFACES(PJ::ParserFactoryPlugin)

public:
  ParserFactoryROS1() = default;

  const char* name() const override
  {
    return "ParserFactoryROS1";
  }
  const char* encoding() const override
  {
    return "ros1msg";
  }

  MessageParserPtr createParser(const std::string& topic_name, const std::string& type_name,
                                const std::string& schema, PlotDataMapRef& data) override
  {
    auto parser = std::make_shared<ParserROS>(topic_name, type_name, schema,
                                              new RosMsgParser::ROS_Deserializer(), data);
    QSettings settings;
    parser->enableTruncationCheck(settings.value("Preferences::truncation_check", true).toBool());
    return parser;
  }
};
