#pragma once

#include "PlotJuggler/messageparser_base.h"

#include <QCheckBox>
#include <QDebug>
#include <string>

#include "rosx_introspection/ros_parser.hpp"
#include "ros_parser.h"
#include "PlotJuggler/fmt/format.h"

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
    return "ros1";
  }

  MessageParserPtr createParser(const std::string& topic_name,
                                const std::string& type_name,
                                const std::string& schema,
                                PlotDataMapRef& data) override
  {
    if(schema.empty())
    {
      throw std::runtime_error("ParserFactoryROS1 requires a schema (message definition)");
    }
    return std::make_shared<ParserROS>(topic_name,  type_name, schema,
                                       new RosMsgParser::ROS_Deserializer(), data);
  }
};



