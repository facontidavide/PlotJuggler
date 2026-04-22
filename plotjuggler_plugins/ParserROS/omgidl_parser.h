#pragma once

#include "PlotJuggler/messageparser_base.h"

#include <QCheckBox>
#include <QSettings>
#include <QDebug>
#include <string>

#include "ros_parser.h"

using namespace PJ;

class ParserFactoryOMGIDL : public ParserFactoryPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.ParserFactoryPlugin")
  Q_INTERFACES(PJ::ParserFactoryPlugin)

public:
  ParserFactoryOMGIDL() = default;

  const char* name() const override
  {
    return "ParserFactoryOMGIDL";
  }
  const char* encoding() const override
  {
    return "omgidl";
  }

  MessageParserPtr createParser(const std::string& topic_name, const std::string& type_name,
                                const std::string& schema, PlotDataMapRef& data) override
  {
    // OMG IDL schemas use scoped names (A::B::C). Normalize the root type to
    // the rosx_introspection convention (A::B/C) so it matches the types
    // produced by ParseIDL.
    std::string msg_type = type_name;
    auto pos = msg_type.rfind("::");
    if (pos != std::string::npos)
    {
      msg_type.replace(pos, 2, "/");
    }

    auto parser = std::make_shared<ParserROS>(topic_name, msg_type, schema,
                                              new RosMsgParser::ROS2_Deserializer(), data,
                                              RosMsgParser::DDS_IDL);
    QSettings settings;
    parser->enableTruncationCheck(settings.value("Preferences::truncation_check", true).toBool());
    return parser;
  }
};
