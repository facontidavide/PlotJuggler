#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

class QDomDocument;
class QDomElement;

namespace PJ::BLF
{

struct BlfPluginConfig
{
  bool emit_raw = true;
  bool emit_decoded = true;
  bool use_source_timestamp = true;
  std::vector<std::string> dbc_files;
  std::map<uint32_t, std::string> channel_to_dbc;
};

void SaveConfigToXml(const BlfPluginConfig& config, QDomDocument& doc, QDomElement& parent);
bool LoadConfigFromXml(const QDomElement& parent, BlfPluginConfig& config,
                       std::string* error = nullptr);

}  // namespace PJ::BLF
