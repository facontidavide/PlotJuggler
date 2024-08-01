#pragma once
#include <PlotJuggler/messageparser_base.h>
#include <QInputDialog>
#include <QDebug>

#ifndef DYNAMIC_CAPNP
#define DYNAMIC_CAPNP  // Do not depend on generated log.capnp.h structure
#endif

#include <capnp/schema-parser.h>
#include <capnp/dynamic.h>
#include <capnp/serialize.h>

#include "common.h"
#include "common_dbc.h"

using namespace PJ;

class RlogMessageParser : MessageParser
{
private:
  std::string dbc_name;
  std::unordered_map<uint8_t, std::shared_ptr<CANParser>> parsers;
  std::shared_ptr<CANPacker> packer;
  bool loadDBC(std::string dbc_str);
  bool show_deprecated;
  bool can_dialog_needed = true;
  const bool streaming;
  capnp::ParsedSchema schema;
  capnp::StructSchema event_struct_schema;

public:
  RlogMessageParser(const std::string& topic_name, PJ::PlotDataMapRef& plot_data, const bool streaming = false);

  capnp::StructSchema getSchema();
  bool parseMessageCereal(capnp::DynamicStruct::Reader event);
  bool parseMessageImpl(const std::string& topic_name, capnp::DynamicValue::Reader node, double timestamp, uint64_t last_sec, bool is_root);
  bool parseCanMessage(const std::string& topic_name, capnp::DynamicList::Reader node, double timestamp, uint64_t last_sec);
  bool parseMessage(const MessageRef serialized_msg, double &timestamp) { return false; };  // not implemented
  void selectDBCDialog();
};
