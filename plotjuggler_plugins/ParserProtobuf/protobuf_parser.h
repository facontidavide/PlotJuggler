#pragma once


#include <QCheckBox>
#include <QDebug>

#include "error_collectors.h"
#include "PlotJuggler/messageparser_base.h"

using namespace PJ;

class ProtobufParser : public MessageParser
{
public:
  ProtobufParser(const std::string& topic_name, PlotDataMapRef& data,
                 const google::protobuf::Descriptor* descriptor)
    : MessageParser(topic_name, data)
    , _msg_descriptor(descriptor)
  {
  }

  bool parseMessage(const MessageRef serialized_msg, double& timestamp) override;

protected:

  google::protobuf::DynamicMessageFactory _msg_factory;
  const google::protobuf::Descriptor* _msg_descriptor;
};



