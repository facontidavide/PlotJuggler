#pragma once


#include <QCheckBox>
#include <QDebug>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/reflection.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/compiler/parser.h>

#include "error_collectors.h"
#include "PlotJuggler/messageparser_base.h"

using namespace PJ;

class ProtobufParser : public MessageParser
{
public:
  ProtobufParser(const std::string& topic_name,
                 const google::protobuf::Descriptor* descriptor,
                 PlotDataMapRef& data)
    : MessageParser(topic_name, data)
    , _proto_pool(&_proto_database)
    , _msg_descriptor(descriptor)
  {
  }

  ProtobufParser(const std::string& topic_name,
                 const std::string type_name,
                 const google::protobuf::FileDescriptorSet& descriptor_set,
                 PlotDataMapRef& data);

  bool parseMessage(const MessageRef serialized_msg, double& timestamp) override;

protected:

  google::protobuf::SimpleDescriptorDatabase _proto_database;
  google::protobuf::DescriptorPool _proto_pool;

  google::protobuf::DynamicMessageFactory _msg_factory;
  const google::protobuf::Descriptor* _msg_descriptor = nullptr;

};



