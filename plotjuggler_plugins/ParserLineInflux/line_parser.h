#pragma once

#include "PlotJuggler/messageparser_base.h"

#include <QCheckBox>
#include <QDebug>
#include <string>

class ParserLine : public PJ::ParserFactoryPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.ParserFactoryPlugin")
  Q_INTERFACES(PJ::ParserFactoryPlugin)

public:
  ParserLine() = default;

  const char* name() const override
  {
    return "ParserLine";
  }
  const char* encoding() const override
  {
    return "Influx (Line protocol)";
  }

  PJ::MessageParserPtr createParser(const std::string& topic_name, const std::string& type_name,
                                    const std::string& schema, PJ::PlotDataMapRef& data) override;
};
