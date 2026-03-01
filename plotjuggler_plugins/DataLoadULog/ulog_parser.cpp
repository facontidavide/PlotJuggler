#include "ulog_parser.h"

#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/reader.hpp>

#include <cstring>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

using BasicType = ulog_cpp::Field::BasicType;

namespace
{

// Size in bytes for each basic type
int basicTypeSize(BasicType type)
{
  switch (type)
  {
    case BasicType::INT8:
    case BasicType::UINT8:
    case BasicType::BOOL:
    case BasicType::CHAR:
      return 1;
    case BasicType::INT16:
    case BasicType::UINT16:
      return 2;
    case BasicType::INT32:
    case BasicType::UINT32:
    case BasicType::FLOAT:
      return 4;
    case BasicType::INT64:
    case BasicType::UINT64:
    case BasicType::DOUBLE:
      return 8;
    default:
      return 0;
  }
}

// Read a basic-type field from raw bytes and return as double
double readAsDouble(const uint8_t* data, int offset, BasicType type)
{
  const uint8_t* p = data + offset;
  switch (type)
  {
    case BasicType::UINT8:
      return static_cast<double>(*p);
    case BasicType::INT8:
      return static_cast<double>(*reinterpret_cast<const int8_t*>(p));
    case BasicType::UINT16: {
      uint16_t v;
      memcpy(&v, p, sizeof(v));
      return static_cast<double>(v);
    }
    case BasicType::INT16: {
      int16_t v;
      memcpy(&v, p, sizeof(v));
      return static_cast<double>(v);
    }
    case BasicType::UINT32: {
      uint32_t v;
      memcpy(&v, p, sizeof(v));
      return static_cast<double>(v);
    }
    case BasicType::INT32: {
      int32_t v;
      memcpy(&v, p, sizeof(v));
      return static_cast<double>(v);
    }
    case BasicType::UINT64: {
      uint64_t v;
      memcpy(&v, p, sizeof(v));
      return static_cast<double>(v);
    }
    case BasicType::INT64: {
      int64_t v;
      memcpy(&v, p, sizeof(v));
      return static_cast<double>(v);
    }
    case BasicType::FLOAT: {
      float v;
      memcpy(&v, p, sizeof(v));
      return static_cast<double>(v);
    }
    case BasicType::DOUBLE: {
      double v;
      memcpy(&v, p, sizeof(v));
      return v;
    }
    case BasicType::BOOL:
      return static_cast<double>(*p != 0);
    case BasicType::CHAR:
      return static_cast<double>(*reinterpret_cast<const char*>(p));
    default:
      return 0.0;
  }
}

// Describes a single leaf field with its full path name and byte offset in the raw sample
struct FlatField
{
  std::string name;
  BasicType type;
  int byte_offset;
};

// Recursively flatten a MessageFormat into leaf fields, skipping timestamp and padding
void flattenFormat(const ulog_cpp::MessageFormat& format, const std::string& prefix,
                   int base_offset, std::vector<FlatField>& out)
{
  for (const auto& field : format.fields())
  {
    // Skip timestamp (handled separately) and padding fields
    if (field->name() == "timestamp")
    {
      continue;
    }
    if (field->name().rfind("_padding", 0) == 0)
    {
      continue;
    }

    std::string new_prefix = prefix + "/" + field->name();
    int count = (field->arrayLength() > 0) ? field->arrayLength() : 1;

    if (field->type().type == BasicType::NESTED)
    {
      int elem_size = field->type().nested_message->sizeBytes();
      for (int i = 0; i < count; i++)
      {
        std::string suffix;
        if (count > 1)
        {
          char buf[16];
          snprintf(buf, sizeof(buf), ".%02d", i);
          suffix = buf;
        }
        int elem_offset = base_offset + field->offsetInMessage() + i * elem_size;
        flattenFormat(*field->type().nested_message, new_prefix + suffix, elem_offset, out);
      }
    }
    else
    {
      int elem_size = basicTypeSize(field->type().type);
      for (int i = 0; i < count; i++)
      {
        std::string suffix;
        if (count > 1)
        {
          char buf[16];
          snprintf(buf, sizeof(buf), ".%02d", i);
          suffix = buf;
        }
        int offset = base_offset + field->offsetInMessage() + i * elem_size;
        out.push_back({ new_prefix + suffix, field->type().type, offset });
      }
    }
  }
}

template <typename T>
std::string intToHex(T i)
{
  std::stringstream stream;
  stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
  return stream.str();
}

}  // namespace

ULogParser::ULogParser(const uint8_t* data, size_t length)
{
  // Parse with ulog_cpp
  auto container =
      std::make_shared<ulog_cpp::DataContainer>(ulog_cpp::DataContainer::StorageConfig::FullLog);
  ulog_cpp::Reader reader(container);
  reader.readChunk(data, static_cast<int>(length));

  if (container->hadFatalError())
  {
    std::string err = "ULog: fatal parsing error";
    if (!container->parsingErrors().empty())
    {
      err += ": " + container->parsingErrors().front();
    }
    throw std::runtime_error(err);
  }

  // --- Convert timeseries ---

  // First pass: find which message names have multi_id > 0
  std::set<std::string> names_with_multi_id;
  for (const auto& [key, sub] : container->subscriptionsByNameAndMultiId())
  {
    if (key.multi_id > 0)
    {
      names_with_multi_id.insert(key.name);
    }
  }

  // Second pass: build timeseries
  for (const auto& [key, sub] : container->subscriptionsByNameAndMultiId())
  {
    // Skip subscriptions with no data samples
    if (sub->rawSamples().empty())
    {
      continue;
    }

    std::string ts_name = key.name;
    if (names_with_multi_id.count(key.name) > 0)
    {
      char buf[16];
      snprintf(buf, sizeof(buf), ".%02d", key.multi_id);
      ts_name += buf;
    }

    const auto& format = *sub->format();

    // Find the timestamp field offset (if present)
    int timestamp_offset = -1;
    try
    {
      auto ts_field = format.field("timestamp");
      if (ts_field->type().type == BasicType::UINT64)
      {
        timestamp_offset = ts_field->offsetInMessage();
      }
    }
    catch (...)
    {
      // No timestamp field
    }

    // Flatten format into leaf fields
    std::vector<FlatField> flat_fields;
    flattenFormat(format, {}, 0, flat_fields);

    // Create timeseries with empty columns
    Timeseries& timeseries = _timeseries[ts_name];
    timeseries.data.reserve(flat_fields.size());
    for (const auto& ff : flat_fields)
    {
      timeseries.data.push_back({ ff.name, std::vector<double>() });
    }

    // Fill data from raw samples
    for (const auto& sample : sub->rawSamples())
    {
      const auto& raw = sample.data();

      // Timestamp
      if (timestamp_offset >= 0 &&
          static_cast<int>(raw.size()) >= timestamp_offset + static_cast<int>(sizeof(uint64_t)))
      {
        uint64_t ts;
        memcpy(&ts, raw.data() + timestamp_offset, sizeof(ts));
        timeseries.timestamps.push_back(ts);
      }
      else
      {
        timeseries.timestamps.push_back(std::nullopt);
      }

      // Field values
      const int raw_size = static_cast<int>(raw.size());
      for (size_t j = 0; j < flat_fields.size(); j++)
      {
        const auto& ff = flat_fields[j];
        int field_end = ff.byte_offset + basicTypeSize(ff.type);
        double val =
            (field_end <= raw_size) ? readAsDouble(raw.data(), ff.byte_offset, ff.type) : 0.0;
        timeseries.data[j].second.push_back(val);
      }
    }
  }

  // --- Convert parameters ---
  // Initial parameters from the definition section
  for (const auto& [name, param] : container->initialParameters())
  {
    Parameter p;
    p.name = name;
    auto bt = param.field().type().type;
    if (bt == BasicType::FLOAT)
    {
      p.value.val_real = param.value().as<float>();
      p.val_type = FLOAT;
    }
    else
    {
      p.value.val_int = param.value().as<int32_t>();
      p.val_type = INT32;
    }
    _parameters.push_back(p);
  }

  // Changed parameters from the data section override initial values
  for (const auto& param : container->changedParameters())
  {
    const std::string& name = param.field().name();
    auto bt = param.field().type().type;

    Parameter new_param;
    new_param.name = name;
    if (bt == BasicType::FLOAT)
    {
      new_param.value.val_real = param.value().as<float>();
      new_param.val_type = FLOAT;
    }
    else
    {
      new_param.value.val_int = param.value().as<int32_t>();
      new_param.val_type = INT32;
    }

    bool found = false;
    for (auto& prev : _parameters)
    {
      if (prev.name == name)
      {
        prev = new_param;
        found = true;
        break;
      }
    }
    if (!found)
    {
      _parameters.push_back(new_param);
    }
  }

  // --- Convert info messages ---
  for (const auto& [name, info] : container->messageInfo())
  {
    auto bt = info.field().type().type;
    std::string value;

    if (info.field().arrayLength() > 0 && bt == BasicType::CHAR)
    {
      // char array → string
      value = info.value().as<std::string>();
    }
    else
    {
      switch (bt)
      {
        case BasicType::BOOL:
          value = std::to_string(info.value().as<int>());
          break;
        case BasicType::UINT8:
          value = std::to_string(info.value().as<uint8_t>());
          break;
        case BasicType::INT8:
          value = std::to_string(info.value().as<int8_t>());
          break;
        case BasicType::UINT16:
          value = std::to_string(info.value().as<uint16_t>());
          break;
        case BasicType::INT16:
          value = std::to_string(info.value().as<int16_t>());
          break;
        case BasicType::UINT32: {
          uint32_t v = info.value().as<uint32_t>();
          if (name.rfind("ver_", 0) == 0 && name.size() > 8 &&
              name.compare(name.size() - 8, 8, "_release") == 0)
          {
            value = intToHex(v);
          }
          else
          {
            value = std::to_string(v);
          }
          break;
        }
        case BasicType::INT32:
          value = std::to_string(info.value().as<int32_t>());
          break;
        case BasicType::FLOAT:
          value = std::to_string(info.value().as<float>());
          break;
        case BasicType::DOUBLE:
          value = std::to_string(info.value().as<double>());
          break;
        case BasicType::UINT64:
          value = std::to_string(info.value().as<uint64_t>());
          break;
        case BasicType::INT64:
          value = std::to_string(info.value().as<int64_t>());
          break;
        default:
          break;
      }
    }
    _info.insert({ name, value });
  }

  // --- Convert logging messages ---
  for (const auto& log : container->logging())
  {
    MessageLog msg;
    msg.level = static_cast<char>(log.logLevel());
    msg.timestamp = log.timestamp();
    msg.msg = log.message();
    _message_logs.push_back(std::move(msg));
  }
}

const std::map<std::string, ULogParser::Timeseries>& ULogParser::getTimeseriesMap() const
{
  return _timeseries;
}

const std::vector<ULogParser::Parameter>& ULogParser::getParameters() const
{
  return _parameters;
}

const std::map<std::string, std::string>& ULogParser::getInfo() const
{
  return _info;
}

const std::vector<ULogParser::MessageLog>& ULogParser::getLogs() const
{
  return _message_logs;
}
