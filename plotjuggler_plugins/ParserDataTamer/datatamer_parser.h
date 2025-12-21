#pragma once

#include "PlotJuggler/messageparser_base.h"

#include <QCheckBox>
#include <QDebug>
#include <string>

class ParserDataTamer : public PJ::ParserFactoryPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.ParserFactoryPlugin")
  Q_INTERFACES(PJ::ParserFactoryPlugin)

public:
  ParserDataTamer() = default;

  const char* name() const override
  {
    return "ParserDataTamer";
  }
  const char* encoding() const override
  {
    return "data_tamer";
  }

  PJ::MessageParserPtr createParser(const std::string& topic_name, const std::string& type_name,
                                    const std::string& schema, PJ::PlotDataMapRef& data) override;
};
