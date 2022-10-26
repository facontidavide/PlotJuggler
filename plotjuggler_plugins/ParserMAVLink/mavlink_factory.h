#ifndef PROTOBUF_FACTORY_H
#define PROTOBUF_FACTORY_H

#include "PlotJuggler/messageparser_base.h"

#include "mavlink_parser.h"

class ParserFactoryMAVLink : public PJ::ParserFactoryPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.ParserFactoryPlugin")
  Q_INTERFACES(PJ::ParserFactoryPlugin)

public:
  ParserFactoryMAVLink() = default;

  const char* name() const override
  {
    return "ParserFactoryMAVLink";
  }
  const char* encoding() const override
  {
    return "mavlink";
  }

  MessageParserPtr createParser(const std::string& topic_name,
                                const std::string& type_name,
                                const std::string& schema,
                                PlotDataMapRef& data) override
  {
    return std::make_shared<MAVLinkParser>(topic_name, data );
  }
};


#endif
