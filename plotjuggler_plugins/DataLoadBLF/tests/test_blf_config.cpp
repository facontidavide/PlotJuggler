#include <gtest/gtest.h>

#include <QDomDocument>
#include <string>
#include <vector>

#include "blf_config.h"

namespace PJ::BLF
{

TEST(BlfConfig, XmlRoundTrip)
{
  BlfPluginConfig input;
  input.emit_raw = true;
  input.emit_decoded = false;
  input.use_source_timestamp = false;
  input.dbc_files = {"powertrain.dbc", "adas.dbc"};
  input.channel_to_dbc = {{1U, "powertrain.dbc"}, {7U, "adas.dbc"}};

  QDomDocument doc("blf_config_test");
  QDomElement root = doc.createElement("plugin");
  doc.appendChild(root);
  SaveConfigToXml(input, doc, root);

  BlfPluginConfig output;
  std::string error;
  ASSERT_TRUE(LoadConfigFromXml(root, output, &error)) << error;

  EXPECT_EQ(output.emit_raw, input.emit_raw);
  EXPECT_EQ(output.emit_decoded, input.emit_decoded);
  EXPECT_EQ(output.use_source_timestamp, input.use_source_timestamp);
  EXPECT_EQ(output.dbc_files, input.dbc_files);
  EXPECT_EQ(output.channel_to_dbc, input.channel_to_dbc);
}

}  // namespace PJ::BLF
