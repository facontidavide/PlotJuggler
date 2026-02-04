#include "data_tamer_parser/data_tamer_parser.hpp"
#include "data_tamer/data_tamer.hpp"
#include "data_tamer/sinks/dummy_sink.hpp"
#include "../examples/geometry_types.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <variant>
#include <string>

using namespace DataTamerParser;
using namespace TestTypes;

TEST(DataTamerParser, ReadSchema)
{
  const char* text = R"(
int8 v1
float64 v2
float32[5] array
int32[] vect
bool is_true
char[256] blob
uint16  my/short
  )";

  const auto schema = BuilSchemaFromText(text);

  ASSERT_EQ(schema.fields.size(), 7);

  TypeField field0 = { "v1", BasicType::INT8, "int8", false, 0 };
  ASSERT_EQ(schema.fields[0], field0);

  TypeField field1 = { "v2", BasicType::FLOAT64, "float64", false, 0 };
  ASSERT_EQ(schema.fields[1], field1);

  TypeField field2 = { "array", BasicType::FLOAT32, "float32", true, 5 };
  ASSERT_EQ(schema.fields[2], field2);

  TypeField field3 = { "vect", BasicType::INT32, "int32", true, 0 };
  ASSERT_EQ(schema.fields[3], field3);

  TypeField field4 = { "is_true", BasicType::BOOL, "bool", false, 0 };
  ASSERT_EQ(schema.fields[4], field4);

  TypeField field5 = { "blob", BasicType::CHAR, "char", true, 256 };
  ASSERT_EQ(schema.fields[5], field5);

  TypeField field6 = { "my/short", BasicType::UINT16, "uint16", false, 0 };
  ASSERT_EQ(schema.fields[6], field6);
}

TEST(DataTamerParser, SchemaHash)
{
  auto channel = DataTamer::LogChannel::create("chan");

  // logs in channelA
  std::vector<double> v1(10, 0);
  std::array<float, 4> v2 = { 1, 2, 3, 4 };
  int32_t v3 = 5;
  uint16_t v4 = 6;
  double v5 = 10;
  uint16_t v6 = 11;
  std::vector<uint8_t> v7(4, 12);
  std::array<uint32_t, 3> v8 = { 13, 14, 15 };

  channel->registerValue("vector_10", &v1);
  channel->registerValue("array_4", &v2);
  channel->registerValue("val_int32", &v3);
  channel->registerValue("val_int16", &v4);
  channel->registerValue("real_value", &v5);
  channel->registerValue("short_int", &v6);
  channel->registerValue("vector_4", &v7);
  channel->registerValue("array_3", &v8);

  const auto schema_in = channel->getSchema();
  std::ostringstream ss;
  ss << schema_in;

  const auto& schema_out = BuilSchemaFromText(ss.str());

  ASSERT_EQ(schema_out.fields[0].field_name, "vector_10");
  ASSERT_EQ(schema_out.fields[1].field_name, "array_4");
  ASSERT_EQ(schema_out.fields[2].field_name, "val_int32");
  ASSERT_EQ(schema_out.fields[3].field_name, "val_int16");
  ASSERT_EQ(schema_out.fields[4].field_name, "real_value");
  ASSERT_EQ(schema_out.fields[5].field_name, "short_int");
  ASSERT_EQ(schema_out.fields[6].field_name, "vector_4");
  ASSERT_EQ(schema_out.fields[7].field_name, "array_3");
}

TEST(DataTamerParser, CustomTypes)
{
  auto channel = DataTamer::LogChannel::create("chan");

  Pose pose;
  channel->registerValue("pose", &pose);

  const auto& schema_in = channel->getSchema();
  const std::string schema_txt = ToStr(schema_in);
  const auto& schema_out = DataTamerParser::BuilSchemaFromText(schema_txt);

  std::cout << schema_txt << std::endl;

  ASSERT_EQ(schema_in.hash, schema_out.hash);
  ASSERT_EQ(schema_in.channel_name, schema_out.channel_name);
  ASSERT_EQ(schema_in.fields.size(), schema_out.fields.size());
  ASSERT_EQ(schema_in.custom_types.size(), schema_out.custom_types.size());

  for(const auto& [type_name, custom_in] : schema_in.custom_types)
  {
    const auto& custom_out = schema_out.custom_types.at(type_name);
    ASSERT_EQ(custom_in.size(), custom_out.size());

    for(size_t i = 0; i < custom_in.size(); i++)
    {
      const auto& field_in = custom_in[i];
      const auto& field_out = custom_out[i];

      ASSERT_EQ(field_in.field_name, field_out.field_name);
      ASSERT_EQ(static_cast<int>(field_in.type), static_cast<int>(field_out.type));
      ASSERT_EQ(field_in.is_vector, field_out.is_vector);
      ASSERT_EQ(field_in.array_size, field_out.array_size);
    }
  }
}

SnapshotView ConvertSnapshot(const DataTamer::Snapshot& snapshot)
{
  return { snapshot.schema_hash,
           uint64_t(snapshot.timestamp.count()),
           { snapshot.active_mask.data(), snapshot.active_mask.size() },
           { snapshot.payload.data(), snapshot.payload.size() } };
}

TEST(DataTamerParser, PlainParsing)
{
  auto channel = DataTamer::LogChannel::create("channel");
  auto dummy_sink = std::make_shared<DataTamer::DummySink>();
  channel->addDataSink(dummy_sink);

  int32_t v1 = 5;
  uint16_t v2 = 6;
  double v3 = 7;
  uint16_t v4 = 8;

  channel->registerValue("v1", &v1);
  channel->registerValue("v2", &v2);
  channel->registerValue("v3", &v3);
  channel->registerValue("v4", &v4);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const auto& schema_in = channel->getSchema();
  const auto& schema_out = DataTamerParser::BuilSchemaFromText(ToStr(schema_in));
  const auto snapshot_view = ConvertSnapshot(dummy_sink->latest_snapshot);

  std::map<std::string, double> parsed_values;
  auto callback = [&](const std::string& field_name,
                      const DataTamerParser::VarNumber& number) {
    parsed_values[field_name] =
        std::visit([](const auto& var) { return double(var); }, number);
  };

  DataTamerParser::ParseSnapshot(schema_out, snapshot_view, callback);

  for(const auto& [name, value] : parsed_values)
  {
    std::cout << name << ": " << value << std::endl;
  }

  ASSERT_EQ(parsed_values.at("v1"), 5);
  ASSERT_EQ(parsed_values.at("v2"), 6);
  ASSERT_EQ(parsed_values.at("v3"), 7);
  ASSERT_EQ(parsed_values.at("v4"), 8);
}

TEST(DataTamerParser, CustomParsing)
{
  DataTamer::ChannelsRegistry registry;
  auto channel = registry.getChannel("channel");
  auto dummy_sink = std::make_shared<DataTamer::DummySink>();
  channel->addDataSink(dummy_sink);

  Pose pose;
  pose.pos = { 1, 2, 3 };
  pose.rot = { 4, 5, 6, 7 };
  channel->registerValue("pose", &pose);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const auto& schema_in = channel->getSchema();
  const auto& schema_out = DataTamerParser::BuilSchemaFromText(ToStr(schema_in));
  const auto snapshot_view = ConvertSnapshot(dummy_sink->latest_snapshot);

  std::map<std::string, double> parsed_values;
  auto callback = [&](const std::string& field_name,
                      const DataTamerParser::VarNumber& number) {
    const double value = std::visit([](const auto& var) { return double(var); }, number);
    parsed_values[field_name] = value;
  };

  DataTamerParser::ParseSnapshot(schema_out, snapshot_view, callback);

  for(const auto& [name, value] : parsed_values)
  {
    std::cout << name << ": " << value << std::endl;
  }

  ASSERT_EQ(parsed_values.at("pose/position/x"), 1);
  ASSERT_EQ(parsed_values.at("pose/position/y"), 2);
  ASSERT_EQ(parsed_values.at("pose/position/z"), 3);

  ASSERT_EQ(parsed_values.at("pose/rotation/w"), 4);
  ASSERT_EQ(parsed_values.at("pose/rotation/x"), 5);
  ASSERT_EQ(parsed_values.at("pose/rotation/y"), 6);
  ASSERT_EQ(parsed_values.at("pose/rotation/z"), 7);
}

TEST(DataTamerParser, VectorParsing)
{
  DataTamer::ChannelsRegistry registry;
  auto channel = registry.getChannel("channel");
  auto dummy_sink = std::make_shared<DataTamer::DummySink>();
  channel->addDataSink(dummy_sink);

  std::vector<double> valsA = { 10, 11, 12 };
  std::array<int, 2> valsB = { 13, 14 };

  std::array<Point3D, 3> points;
  points[0] = { 1, 2, 3 };
  points[1] = { 4, 5, 6 };
  points[2] = { 7, 8, 9 };
  std::vector<Quaternion> quats(2);
  quats[0] = { 20, 21, 22, 23 };
  quats[1] = { 30, 31, 32, 33 };

  channel->registerValue("valsA", &valsA);
  channel->registerValue("valsB", &valsB);
  channel->registerValue("points", &points);
  channel->registerValue("quats", &quats);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const auto& schema_in = channel->getSchema();
  const auto& schema_out = DataTamerParser::BuilSchemaFromText(ToStr(schema_in));
  const auto snapshot_view = ConvertSnapshot(dummy_sink->latest_snapshot);

  std::map<std::string, double> parsed_values;
  auto callback = [&](const std::string& field_name,
                      const DataTamerParser::VarNumber& number) {
    const double value = std::visit([](const auto& var) { return double(var); }, number);
    parsed_values[field_name] = value;
  };

  DataTamerParser::ParseSnapshot(schema_out, snapshot_view, callback);

  for(const auto& [name, value] : parsed_values)
  {
    std::cout << name << ": " << value << std::endl;
  }

  ASSERT_EQ(parsed_values.at("valsA[0]"), 10);
  ASSERT_EQ(parsed_values.at("valsA[1]"), 11);
  ASSERT_EQ(parsed_values.at("valsA[2]"), 12);

  ASSERT_EQ(parsed_values.at("valsB[0]"), 13);
  ASSERT_EQ(parsed_values.at("valsB[1]"), 14);

  ASSERT_EQ(parsed_values.at("points[0]/x"), 1);
  ASSERT_EQ(parsed_values.at("points[0]/y"), 2);
  ASSERT_EQ(parsed_values.at("points[0]/z"), 3);

  ASSERT_EQ(parsed_values.at("points[1]/x"), 4);
  ASSERT_EQ(parsed_values.at("points[1]/y"), 5);
  ASSERT_EQ(parsed_values.at("points[1]/z"), 6);

  ASSERT_EQ(parsed_values.at("points[2]/x"), 7);
  ASSERT_EQ(parsed_values.at("points[2]/y"), 8);
  ASSERT_EQ(parsed_values.at("points[2]/z"), 9);

  ASSERT_EQ(parsed_values.at("quats[0]/w"), 20);
  ASSERT_EQ(parsed_values.at("quats[0]/x"), 21);
  ASSERT_EQ(parsed_values.at("quats[0]/y"), 22);
  ASSERT_EQ(parsed_values.at("quats[0]/z"), 23);

  ASSERT_EQ(parsed_values.at("quats[1]/w"), 30);
  ASSERT_EQ(parsed_values.at("quats[1]/x"), 31);
  ASSERT_EQ(parsed_values.at("quats[1]/y"), 32);
  ASSERT_EQ(parsed_values.at("quats[1]/z"), 33);
}
