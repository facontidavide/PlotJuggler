#pragma once

#include <PlotJuggler/messageparser_base.h>
#include <capnp/dynamic.h>
#include <iostream>

using namespace PJ;

class RlogMessageParser : MessageParser
{
public:
  RlogMessageParser(const std::string& topic_name, PJ::PlotDataMapRef& plot_data):
    MessageParser(topic_name, plot_data) {}

  bool parseMessageImpl(const std::string& topic_name, capnp::DynamicValue::Reader node, double timestamp);
  bool parseMessage(const MessageRef serialized_msg, double timestamp);
};
