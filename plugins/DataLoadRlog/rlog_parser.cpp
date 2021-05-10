#include "rlog_parser.hpp"

bool RlogMessageParser::loadDBC(std::string dbc_str) {
  if (!dbc_str.empty()) {
    if (dbc_lookup(dbc_str) == nullptr) {
      return false;
    }
    dbc_name = dbc_str;  // is used later to instantiate CANParser
    packer = std::make_shared<CANPacker>(dbc_name);
  }
  return true;
}

bool RlogMessageParser::parseMessage(const MessageRef msg, double time_stamp)
{
  return false;
}

bool RlogMessageParser::parseMessageImpl(const std::string& topic_name, capnp::DynamicValue::Reader value, double time_stamp, bool show_deprecated)
{

  PJ::PlotData& _data_series = getSeries(topic_name);

  switch (value.getType()) 
  {
    case capnp::DynamicValue::BOOL: 
    {
      _data_series.pushBack({time_stamp, (double)value.as<bool>()});
      break;
    }

    case capnp::DynamicValue::INT: 
    {
      _data_series.pushBack({time_stamp, (double)value.as<int64_t>()});
      break;
    }

    case capnp::DynamicValue::UINT: 
    {
      _data_series.pushBack({time_stamp, (double)value.as<uint64_t>()});
      break;
    }

    case capnp::DynamicValue::FLOAT: 
    {
      _data_series.pushBack({time_stamp, (double)value.as<double>()});
      break;
    }

    case capnp::DynamicValue::LIST: 
    {
      // TODO: Parse lists properly
      int i = 0;
      for(auto element : value.as<capnp::DynamicList>())
      {
        parseMessageImpl(topic_name + '/' + std::to_string(i), element, time_stamp, show_deprecated);
        i++;
      }
      break;
    }

    case capnp::DynamicValue::ENUM: 
    {
      auto enumValue = value.as<capnp::DynamicEnum>();
      _data_series.pushBack({time_stamp, (double)enumValue.getRaw()});
      break;
    }

    case capnp::DynamicValue::STRUCT: 
    {
      auto structValue = value.as<capnp::DynamicStruct>();
      for (auto field : structValue.getSchema().getFields()) 
      {
        if (structValue.has(field))
        {
          std::string name = field.getProto().getName();
          if (show_deprecated || name.find("DEPRECATED") == std::string::npos) {
            parseMessageImpl(topic_name + '/' + name, structValue.get(field), time_stamp, show_deprecated);
          }
        }
      }
      break;
    }

    default:
    {
      // We currently don't support: DATA, ANY_POINTER, TEXT, CAPABILITIES, VOID
      break;
    }
  }
  return true;
}

bool RlogMessageParser::parseCanMessage(
  const std::string& topic_name, capnp::DynamicList::Reader listValue, double time_stamp) 
{
  if (dbc_name.empty()) {
    return false;
  }
  std::set<uint8_t> updated_busses;
  for(auto elem : listValue) {
    auto value = elem.as<capnp::DynamicStruct>();
    uint8_t bus = value.get("src").as<uint8_t>();
    if (parsers.find(bus) == parsers.end()) {
      parsers[bus] = std::make_shared<CANParser>(bus, dbc_name, true, true);
      parsers[bus]->last_sec = 1;
    }

    updated_busses.insert(bus);
    parsers[bus]->UpdateCans((uint64_t)(time_stamp), value);
  }
  for (uint8_t bus : updated_busses) {
    for (auto& sg : parsers[bus]->query_latest()) {
      PJ::PlotData& _data_series = getSeries(topic_name + '/' + std::to_string(bus) + '/' + 
          packer->lookup_message(sg.address)->name + '/' + sg.name);
      _data_series.pushBack({time_stamp, (double)sg.value});
    }
    parsers[bus]->last_sec = (uint64_t)(time_stamp);
  }
  return true;
}