#include "dataload_ulog2.h"

#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/reader.hpp>

#include <QDebug>
#include <QFile>
#include <QMainWindow>
#include <QMessageBox>
#include <QSettings>
#include <QWidget>

#include "ulog_parameters_dialog.h"

#include <cstring>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

using BasicType = ulog_cpp::Field::BasicType;

namespace
{

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

struct FlatField
{
  std::string name;
  BasicType type;
  int byte_offset;
};

void flattenFormat(const ulog_cpp::MessageFormat& format, const std::string& prefix,
                   int base_offset, std::vector<FlatField>& out)
{
  for (const auto& field : format.fields())
  {
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

double extractParamValue(const ulog_cpp::MessageInfo& param)
{
  auto bt = param.field().type().type;
  if (bt == BasicType::FLOAT)
  {
    return static_cast<double>(param.value().as<float>());
  }
  return static_cast<double>(param.value().as<int32_t>());
}

double extractParamDefaultValue(const ulog_cpp::ParameterDefault& param)
{
  auto bt = param.field().type().type;
  if (bt == BasicType::FLOAT)
  {
    return static_cast<double>(param.value().as<float>());
  }
  return static_cast<double>(param.value().as<int32_t>());
}

template <typename T>
std::string intToHex(T i)
{
  std::stringstream stream;
  stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
  return stream.str();
}

std::string infoValueToString(const std::string& name, const ulog_cpp::MessageInfo& info)
{
  auto bt = info.field().type().type;

  if (info.field().arrayLength() > 0 && bt == BasicType::CHAR)
  {
    return info.value().as<std::string>();
  }

  switch (bt)
  {
    case BasicType::BOOL:
      return std::to_string(info.value().as<int>());
    case BasicType::UINT8:
      return std::to_string(info.value().as<uint8_t>());
    case BasicType::INT8:
      return std::to_string(info.value().as<int8_t>());
    case BasicType::UINT16:
      return std::to_string(info.value().as<uint16_t>());
    case BasicType::INT16:
      return std::to_string(info.value().as<int16_t>());
    case BasicType::UINT32: {
      uint32_t v = info.value().as<uint32_t>();
      if (name.rfind("ver_", 0) == 0 && name.size() > 8 &&
          name.compare(name.size() - 8, 8, "_release") == 0)
      {
        return intToHex(v);
      }
      return std::to_string(v);
    }
    case BasicType::INT32:
      return std::to_string(info.value().as<int32_t>());
    case BasicType::FLOAT:
      return std::to_string(info.value().as<float>());
    case BasicType::DOUBLE:
      return std::to_string(info.value().as<double>());
    case BasicType::UINT64:
      return std::to_string(info.value().as<uint64_t>());
    case BasicType::INT64:
      return std::to_string(info.value().as<int64_t>());
    default:
      return {};
  }
}

}  // namespace

DataLoadULog2::DataLoadULog2() : _main_win(nullptr)
{
  for (QWidget* widget : qApp->topLevelWidgets())
  {
    if (widget->inherits("QMainWindow"))
    {
      _main_win = widget;
      break;
    }
  }
}

const std::vector<const char*>& DataLoadULog2::compatibleFileExtensions() const
{
  static std::vector<const char*> extensions = { "ulg" };
  return extensions;
}

bool DataLoadULog2::readDataFromFile(FileLoadInfo* fileload_info, PlotDataMapRef& plot_data)
{
  const auto& filename = fileload_info->filename;

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly))
  {
    throw std::runtime_error("ULog: Failed to open file");
  }
  QByteArray file_array = file.readAll();

  // Parse with ulog_cpp
  auto container =
      std::make_shared<ulog_cpp::DataContainer>(ulog_cpp::DataContainer::StorageConfig::FullLog);
  ulog_cpp::Reader reader(container);
  reader.readChunk(reinterpret_cast<const uint8_t*>(file_array.data()),
                   static_cast<int>(file_array.size()));

  if (container->hadFatalError())
  {
    std::string err = "ULog: fatal parsing error";
    if (!container->parsingErrors().empty())
    {
      err += ": " + container->parsingErrors().front();
    }
    throw std::runtime_error(err);
  }

  // --- Timeseries ---

  // Find which message names have multi_id > 0
  std::set<std::string> names_with_multi_id;
  for (const auto& [key, sub] : container->subscriptionsByNameAndMultiId())
  {
    if (key.multi_id > 0)
    {
      names_with_multi_id.insert(key.name);
    }
  }

  auto min_msg_time = std::numeric_limits<double>::max();

  for (const auto& [key, sub] : container->subscriptionsByNameAndMultiId())
  {
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

    // Find the timestamp field offset
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
    }

    // Flatten format into leaf fields
    std::vector<FlatField> flat_fields;
    flattenFormat(format, {}, 0, flat_fields);

    auto group = plot_data.getOrCreateGroup(ts_name);

    // Create numeric series for each field
    std::vector<PlotData*> series_ptrs;
    series_ptrs.reserve(flat_fields.size());
    for (const auto& ff : flat_fields)
    {
      auto it = plot_data.addNumeric(ts_name + ff.name, group);
      series_ptrs.push_back(&it->second);
    }

    // Fill data from raw samples
    size_t sample_index = 0;
    for (const auto& sample : sub->rawSamples())
    {
      const auto& raw = sample.data();
      const int raw_size = static_cast<int>(raw.size());

      // Timestamp (fall back to sample index if no timestamp field)
      double msg_time;
      if (timestamp_offset >= 0 &&
          raw_size >= timestamp_offset + static_cast<int>(sizeof(uint64_t)))
      {
        uint64_t ts;
        memcpy(&ts, raw.data() + timestamp_offset, sizeof(ts));
        msg_time = static_cast<double>(ts) * 0.000001;
      }
      else
      {
        msg_time = static_cast<double>(sample_index);
      }
      min_msg_time = std::min(min_msg_time, msg_time);

      // Field values
      for (size_t j = 0; j < flat_fields.size(); j++)
      {
        const auto& ff = flat_fields[j];
        int field_end = ff.byte_offset + basicTypeSize(ff.type);
        double val =
            (field_end <= raw_size) ? readAsDouble(raw.data(), ff.byte_offset, ff.type) : 0.0;
        series_ptrs[j]->pushBack({ msg_time, val });
      }
      sample_index++;
    }
  }

  if (min_msg_time == std::numeric_limits<double>::max())
  {
    min_msg_time = 0.0;
  }

  // --- Log messages as StringSeries ---
  if (!container->logging().empty())
  {
    auto log_it = plot_data.addStringSeries("_ulog/logs");
    for (const auto& log : container->logging())
    {
      double t = static_cast<double>(log.timestamp()) * 0.000001;
      std::string text = "[" + log.logLevelStr() + "] " + log.message();
      if (log.hasTag())
      {
        text = "[tag=" + std::to_string(log.tag()) + "] " + text;
      }
      log_it->second.pushBack({ t, text });
    }
  }

  // --- Dropouts as numeric series ---
  if (!container->dropouts().empty())
  {
    // Dropouts don't have timestamps in ULog, so we number them sequentially
    auto dropout_it = plot_data.addNumeric("_ulog/dropout_duration_ms");
    int idx = 0;
    for (const auto& d : container->dropouts())
    {
      dropout_it->second.pushBack(
          { static_cast<double>(idx), static_cast<double>(d.durationMs()) });
      idx++;
    }
  }

  // --- Parameters as single-point series ---
  for (const auto& [name, param] : container->initialParameters())
  {
    double val = extractParamValue(param);
    auto series = plot_data.addNumeric("_parameters/" + name);
    series->second.pushBack({ min_msg_time, val });
  }

  // Changed parameters override initial values (last change wins)
  std::map<std::string, double> changed_values;
  for (const auto& param : container->changedParameters())
  {
    changed_values[param.field().name()] = extractParamValue(param);
  }
  for (const auto& [name, val] : changed_values)
  {
    auto it = plot_data.numeric.find("_parameters/" + name);
    if (it != plot_data.numeric.end())
    {
      it->second.clear();
      it->second.pushBack({ min_msg_time, val });
    }
    else
    {
      auto series = plot_data.addNumeric("_parameters/" + name);
      series->second.pushBack({ min_msg_time, val });
    }
  }

  // --- Show dialog ---
  auto* dialog = new ULogParametersDialog(*container, _main_win);
  dialog->setWindowTitle(QString("ULog file %1").arg(filename));
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->restoreSettings();
  dialog->show();

  return true;
}

DataLoadULog2::~DataLoadULog2()
{
}

bool DataLoadULog2::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  return true;
}

bool DataLoadULog2::xmlLoadState(const QDomElement&)
{
  return true;
}
