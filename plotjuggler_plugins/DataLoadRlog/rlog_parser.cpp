#include "rlog_parser.hpp"

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <QDir>
#include <QString>

RlogMessageParser::RlogMessageParser(const std::string& topic_name, PJ::PlotDataMapRef& plot_data, const bool streaming) :
  MessageParser(topic_name, plot_data), streaming(streaming)
{
  show_deprecated = std::getenv("SHOW_DEPRECATED");
  if (std::getenv("DBC_NAME") != nullptr)
  {
    can_dialog_needed = !loadDBC(std::getenv("DBC_NAME"));
  }


  // load schema
  QString schema_path(std::getenv("BASEDIR"));

  if (schema_path.isNull())
  {
    schema_path = QDir(getpwuid(getuid())->pw_dir).filePath("openpilot"); // fallback to $HOME/openpilot
  }
  schema_path = QDir(schema_path).filePath("cereal/log.capnp");
  qDebug() << "Loading schema:" << schema_path;
  schema_path.remove(0, 1);

  // Parse the schema
  auto fs = kj::newDiskFilesystem();
  capnp::SchemaParser *schema_parser = new capnp::SchemaParser;
  this->schema = schema_parser->parseFromDirectory(fs->getRoot(), kj::Path::parse(schema_path.toStdString()), nullptr);
  this->event_struct_schema = schema.getNested("Event").asStruct();
}

capnp::StructSchema RlogMessageParser::getSchema()
{
  return event_struct_schema;
}

bool RlogMessageParser::loadDBC(std::string dbc_str)
{
  if (!dbc_str.empty() && dbc_lookup(dbc_str) != nullptr) {
    dbc_name = dbc_str;  // is used later to instantiate CANParser
    packer = std::make_shared<CANPacker>(dbc_name);
    qDebug() << "Loaded DBC:" << dbc_name.c_str();
    return true;
  }
  qDebug() << "Could not load specified DBC file:" << dbc_str.c_str();
  return false;
}

bool RlogMessageParser::parseMessageCereal(capnp::DynamicStruct::Reader event)
{
  if (can_dialog_needed && !streaming && (event.has("can") || event.has("sendcan"))) {
    selectDBCDialog();  // prompts for and loads DBC
  }

  double time_stamp = (double)event.get("logMonoTime").as<uint64_t>() / 1e9;
  if (event.has("can")) {
    return parseCanMessage("/can", event.get("can").as<capnp::DynamicList>(), time_stamp, event.get("logMonoTime").as<uint64_t>());
  } else if (event.has("sendcan")) {
    return parseCanMessage("/sendcan", event.get("sendcan").as<capnp::DynamicList>(), time_stamp, event.get("logMonoTime").as<uint64_t>());
  } else {
    return parseMessageImpl("", event, time_stamp, true);
  }
}

bool RlogMessageParser::parseMessageImpl(const std::string& topic_name, capnp::DynamicValue::Reader value, double time_stamp, bool is_root)
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
        parseMessageImpl(topic_name + '/' + std::to_string(i), element, time_stamp, false);
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
      std::string structName;
      KJ_IF_MAYBE(e_, structValue.which())
      {
        structName = e_->getProto().getName();
      }
      // skips root structs that are deprecated
      if (!show_deprecated && structName.find("DEPRECATED") != std::string::npos)
      {
        break;
      }

      const int offset = structValue.getSchema().getProto().getStruct().getDiscriminantCount();
      for (const auto &field : structValue.getSchema().getFields())
      {
        std::string name = field.getProto().getName();
        if (structValue.has(field) && (show_deprecated || name.find("DEPRECATED") == std::string::npos))
        {
          // field is in a union if discriminant is less than the size of the union
          // https://github.com/capnproto/capnproto/blob/master/c++/src/capnp/schema.capnp
          const bool in_union = field.getProto().getDiscriminantValue() < offset;

          if (!is_root || in_union)
          {
            parseMessageImpl(topic_name + '/' + name, structValue.get(field), time_stamp, false);
          }
          else if (is_root && !in_union)
          {
            parseMessageImpl(topic_name + '/' + structName + "/__" + name, structValue.get(field), time_stamp, false);
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
  const std::string& topic_name, capnp::DynamicList::Reader listValue, double time_stamp, uint64_t _log_mono_time)
{
  if (dbc_name.empty()) {
    return false;
  }
  // TODO: see if the way parser.cc does it is faster or not
  std::set<uint8_t> updated_busses;
  for(auto elem : listValue) {
    auto value = elem.as<capnp::DynamicStruct>();
    uint8_t bus = value.get("src").as<uint8_t>();
    if (parsers.find(bus) == parsers.end()) {
      parsers[bus] = std::make_shared<CANParser>(bus, dbc_name, true, true);
      parsers[bus]->last_sec = 1;
    }

    updated_busses.insert(bus);
    parsers[bus]->UpdateCans(_log_mono_time, value);
  }
  for (uint8_t bus : updated_busses) {
    // TODO: use true monologtime
    parsers[bus]->last_sec = _log_mono_time;
    for (auto& sg : parsers[bus]->query_latest()) {
      // TODO: use all values
      // qDebug() << sg.all_values;
      PJ::PlotData& _data_series = getSeries(topic_name + '/' + std::to_string(bus) + '/' +
          packer->lookup_message(sg.address)->name + '/' + sg.name);
      _data_series.pushBack({time_stamp, (double)sg.value});
    }
//    qDebug() << time_stamp;
//    qDebug() << time_stamp * 1e9;

  }
  return true;
}

void RlogMessageParser::selectDBCDialog() {
  if (can_dialog_needed)
  {
    QStringList dbc_items;
    dbc_items.append("");
    for (auto dbc : get_dbcs()) {
      dbc_items.append(dbc->name);
    }
    bool dbc_selected;
    QString selected_str = QInputDialog::getItem(
      nullptr, QObject::tr("Select DBC"), QObject::tr("Parse CAN using DBC:"), dbc_items, 0, false, &dbc_selected);
    if (dbc_selected && !selected_str.isEmpty()) {
      can_dialog_needed = !loadDBC(selected_str.toStdString());
    }
  }
}
