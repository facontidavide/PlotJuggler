#pragma once

#include "PlotJuggler/messageparser_base.h"

#include <QCheckBox>
#include <QDebug>
#include <string>

#include "rosx_introspection/ros_parser.hpp"

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
                                PlotDataMapRef& data) override;
};



