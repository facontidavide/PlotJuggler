#include "blf_config.h"

#include <QDomDocument>
#include <QDomElement>

#include <sstream>

namespace PJ::BLF
{
namespace
{
constexpr const char* kConfigTag = "blf_config";
constexpr const char* kDbcFilesTag = "dbc_files";
constexpr const char* kFileTag = "file";
constexpr const char* kChannelBindingsTag = "channel_to_dbc";
constexpr const char* kBindingTag = "binding";

bool ReadBoolAttribute(const QDomElement& element, const char* name, bool default_value)
{
  const QString raw = element.attribute(name);
  if (raw.isEmpty())
  {
    return default_value;
  }

  if (raw == "1" || raw.compare("true", Qt::CaseInsensitive) == 0)
  {
    return true;
  }
  if (raw == "0" || raw.compare("false", Qt::CaseInsensitive) == 0)
  {
    return false;
  }
  return default_value;
}

void SetError(std::string* error, const std::string& message)
{
  if (error)
  {
    *error = message;
  }
}
}  // namespace

void SaveConfigToXml(const BlfPluginConfig& config, QDomDocument& doc, QDomElement& parent)
{
  QDomElement cfg = doc.createElement(kConfigTag);
  cfg.setAttribute("emit_raw", config.emit_raw ? "1" : "0");
  cfg.setAttribute("emit_decoded", config.emit_decoded ? "1" : "0");
  cfg.setAttribute("use_source_timestamp", config.use_source_timestamp ? "1" : "0");

  QDomElement dbc_files_elem = doc.createElement(kDbcFilesTag);
  for (const auto& dbc_file : config.dbc_files)
  {
    QDomElement file_elem = doc.createElement(kFileTag);
    file_elem.setAttribute("path", QString::fromStdString(dbc_file));
    dbc_files_elem.appendChild(file_elem);
  }
  cfg.appendChild(dbc_files_elem);

  QDomElement bindings_elem = doc.createElement(kChannelBindingsTag);
  for (const auto& [channel, dbc_file] : config.channel_to_dbc)
  {
    QDomElement binding_elem = doc.createElement(kBindingTag);
    binding_elem.setAttribute("channel", QString::number(channel));
    binding_elem.setAttribute("dbc", QString::fromStdString(dbc_file));
    bindings_elem.appendChild(binding_elem);
  }
  cfg.appendChild(bindings_elem);

  parent.appendChild(cfg);
}

bool LoadConfigFromXml(const QDomElement& parent, BlfPluginConfig& config, std::string* error)
{
  const QDomElement cfg = parent.firstChildElement(kConfigTag);
  if (cfg.isNull())
  {
    SetError(error, "missing <blf_config> element");
    return false;
  }

  config.emit_raw = ReadBoolAttribute(cfg, "emit_raw", true);
  config.emit_decoded = ReadBoolAttribute(cfg, "emit_decoded", true);
  config.use_source_timestamp = ReadBoolAttribute(cfg, "use_source_timestamp", true);
  config.dbc_files.clear();
  config.channel_to_dbc.clear();

  const QDomElement dbc_files_elem = cfg.firstChildElement(kDbcFilesTag);
  for (QDomElement file_elem = dbc_files_elem.firstChildElement(kFileTag); !file_elem.isNull();
       file_elem = file_elem.nextSiblingElement(kFileTag))
  {
    const QString path = file_elem.attribute("path");
    if (!path.isEmpty())
    {
      config.dbc_files.push_back(path.toStdString());
    }
  }

  const QDomElement bindings_elem = cfg.firstChildElement(kChannelBindingsTag);
  for (QDomElement binding_elem = bindings_elem.firstChildElement(kBindingTag);
       !binding_elem.isNull(); binding_elem = binding_elem.nextSiblingElement(kBindingTag))
  {
    bool ok = false;
    const uint32_t channel = binding_elem.attribute("channel").toUInt(&ok);
    if (!ok)
    {
      SetError(error, "invalid channel value in <binding>");
      return false;
    }
    const std::string dbc_file = binding_elem.attribute("dbc").toStdString();
    if (dbc_file.empty())
    {
      std::ostringstream oss;
      oss << "empty dbc attribute for channel " << channel;
      SetError(error, oss.str());
      return false;
    }
    const auto inserted = config.channel_to_dbc.emplace(channel, dbc_file);
    if (!inserted.second)
    {
      std::ostringstream oss;
      oss << "duplicate channel binding in xml: " << channel;
      SetError(error, oss.str());
      return false;
    }
  }

  return true;
}

}  // namespace PJ::BLF
