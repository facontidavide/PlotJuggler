#pragma once

#include "PlotJuggler/messageparser_base.h"

#include <QCheckBox>
#include <QDebug>
#include <QSettings>
#include <string>

using namespace PJ;

class ParserFactoryIDL : public ParserFactoryPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.ParserFactoryPlugin")
  Q_INTERFACES(PJ::ParserFactoryPlugin)

public:
  ParserFactoryIDL() = default;

  const char* name() const override
  {
    return "ParserFactoryIDL";
  }
  const char* encoding() const override
  {
    return "omgidl";
  }

  MessageParserPtr createParser(const std::string& topic_name,
                                const std::string& type_name, const std::string& schema,
                                PlotDataMapRef& data) override;
};
