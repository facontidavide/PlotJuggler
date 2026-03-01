#include <gtest/gtest.h>
#include "ulog_parser.h"
#include <ulog_cpp/raw_messages.hpp>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using ulog_cpp::ULogMessageType;

// ---------------------------------------------------------------------------
// Helper: builds raw ULog binary buffers for testing
// ---------------------------------------------------------------------------
class ULogBufferBuilder
{
  std::vector<char> buf_;

  void appendBytes(const void* data, size_t len)
  {
    auto p = static_cast<const char*>(data);
    buf_.insert(buf_.end(), p, p + len);
  }

  template <typename T>
  void appendLE(T v)
  {
    appendBytes(&v, sizeof(v));
  }

  void appendMsgHeader(uint16_t msg_size, uint8_t msg_type)
  {
    appendLE<uint16_t>(msg_size);
    buf_.push_back(static_cast<char>(msg_type));
  }

public:
  explicit ULogBufferBuilder(uint64_t timestamp = 0)
  {
    const uint8_t magic[] = { 'U', 'L', 'o', 'g', 0x01, 0x12, 0x35, 0x00 };
    appendBytes(magic, 8);
    appendLE(timestamp);
  }

  void addFlagBits()
  {
    appendMsgHeader(40, static_cast<uint8_t>(ULogMessageType::FLAG_BITS));
    for (int i = 0; i < 40; i++)
    {
      buf_.push_back(0);
    }
  }

  void addFormat(const std::string& fmt)
  {
    appendMsgHeader(static_cast<uint16_t>(fmt.size()),
                    static_cast<uint8_t>(ULogMessageType::FORMAT));
    appendBytes(fmt.data(), fmt.size());
  }

  void addInfo(const std::string& typed_key, const void* value, size_t value_len)
  {
    uint8_t key_len = static_cast<uint8_t>(typed_key.size());
    uint16_t msg_size = 1 + key_len + static_cast<uint16_t>(value_len);
    appendMsgHeader(msg_size, static_cast<uint8_t>(ULogMessageType::INFO));
    buf_.push_back(static_cast<char>(key_len));
    appendBytes(typed_key.data(), key_len);
    appendBytes(value, value_len);
  }

  void addInfoString(const std::string& name, const std::string& value)
  {
    std::string typed_key = "char[" + std::to_string(value.size()) + "] " + name;
    addInfo(typed_key, value.data(), value.size());
  }

  template <typename T>
  void addInfoNumeric(const std::string& type_name, const std::string& key_name, T value)
  {
    std::string typed_key = type_name + " " + key_name;
    addInfo(typed_key, &value, sizeof(value));
  }

  void addParameter(const std::string& type, const std::string& name, const void* value,
                    size_t value_len)
  {
    std::string key = type + " " + name;
    uint8_t key_len = static_cast<uint8_t>(key.size());
    uint16_t msg_size = 1 + key_len + static_cast<uint16_t>(value_len);
    appendMsgHeader(msg_size, static_cast<uint8_t>(ULogMessageType::PARAMETER));
    buf_.push_back(static_cast<char>(key_len));
    appendBytes(key.data(), key_len);
    appendBytes(value, value_len);
  }

  void addParameterFloat(const std::string& name, float value)
  {
    addParameter("float", name, &value, sizeof(value));
  }

  void addParameterInt(const std::string& name, int32_t value)
  {
    addParameter("int32_t", name, &value, sizeof(value));
  }

  void addParameterDefault(const std::string& type, const std::string& name, const void* value,
                           size_t value_len)
  {
    std::string key = type + " " + name;
    uint8_t key_len = static_cast<uint8_t>(key.size());
    uint16_t msg_size = 1 + 1 + key_len + static_cast<uint16_t>(value_len);
    appendMsgHeader(msg_size, static_cast<uint8_t>(ULogMessageType::PARAMETER_DEFAULT));
    buf_.push_back(0x01);
    buf_.push_back(static_cast<char>(key_len));
    appendBytes(key.data(), key_len);
    appendBytes(value, value_len);
  }

  void addSubscription(uint16_t msg_id, uint8_t multi_id, const std::string& name)
  {
    uint16_t msg_size = 3 + static_cast<uint16_t>(name.size());
    appendMsgHeader(msg_size, static_cast<uint8_t>(ULogMessageType::ADD_LOGGED_MSG));
    buf_.push_back(static_cast<char>(multi_id));
    appendLE(msg_id);
    appendBytes(name.data(), name.size());
  }

  void addData(uint16_t msg_id, const std::vector<char>& payload)
  {
    uint16_t msg_size = 2 + static_cast<uint16_t>(payload.size());
    appendMsgHeader(msg_size, static_cast<uint8_t>(ULogMessageType::DATA));
    appendLE(msg_id);
    appendBytes(payload.data(), payload.size());
  }

  void addLogging(char level, uint64_t timestamp, const std::string& text)
  {
    uint16_t msg_size = 1 + 8 + static_cast<uint16_t>(text.size());
    appendMsgHeader(msg_size, static_cast<uint8_t>(ULogMessageType::LOGGING));
    buf_.push_back(level);
    appendLE(timestamp);
    appendBytes(text.data(), text.size());
  }

  char* data()
  {
    return buf_.data();
  }
  int size() const
  {
    return static_cast<int>(buf_.size());
  }
};

// ---------------------------------------------------------------------------
// Helper: builds DATA message payloads field by field
// ---------------------------------------------------------------------------
class DataPayloadBuilder
{
  std::vector<char> p_;

  template <typename T>
  void append(T v)
  {
    auto ptr = reinterpret_cast<const char*>(&v);
    p_.insert(p_.end(), ptr, ptr + sizeof(T));
  }

public:
  DataPayloadBuilder& u8(uint8_t v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& u16(uint16_t v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& u32(uint32_t v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& u64(uint64_t v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& i8(int8_t v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& i16(int16_t v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& i32(int32_t v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& i64(int64_t v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& f32(float v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& f64(double v)
  {
    append(v);
    return *this;
  }
  DataPayloadBuilder& boolean(bool v)
  {
    p_.push_back(v ? 1 : 0);
    return *this;
  }
  DataPayloadBuilder& ch(char v)
  {
    p_.push_back(v);
    return *this;
  }
  DataPayloadBuilder& padding(int n)
  {
    for (int i = 0; i < n; i++)
    {
      p_.push_back(0);
    }
    return *this;
  }
  DataPayloadBuilder& timestamp(uint64_t v)
  {
    return u64(v);
  }
  std::vector<char> build() const
  {
    return p_;
  }
};

// ---------------------------------------------------------------------------
// Helper: parse buffer into ULogParser
// ---------------------------------------------------------------------------
static std::unique_ptr<ULogParser> parseBuffer(ULogBufferBuilder& b)
{
  return std::make_unique<ULogParser>(reinterpret_cast<const uint8_t*>(b.data()), b.size());
}

// ===========================================================================
// Basic parsing
// ===========================================================================

TEST(ULogParserTest, MinimalValidFile)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("sensor:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "sensor");
  b.addData(0, DataPayloadBuilder().timestamp(1000).f32(3.14f).build());

  auto parser = parseBuffer(b);
  auto& ts_map = parser->getTimeseriesMap();
  ASSERT_EQ(ts_map.size(), 1u);

  auto& ts = ts_map.at("sensor");
  ASSERT_EQ(ts.timestamps.size(), 1u);
  ASSERT_TRUE(ts.timestamps[0].has_value());
  EXPECT_EQ(ts.timestamps[0].value(), 1000u);
  ASSERT_EQ(ts.data.size(), 1u);
  EXPECT_EQ(ts.data[0].first, "/x");
  EXPECT_FLOAT_EQ(ts.data[0].second[0], 3.14f);
}

TEST(ULogParserTest, MultipleDataMessages)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("sensor:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "sensor");
  for (int i = 0; i < 10; i++)
  {
    b.addData(0, DataPayloadBuilder()
                     .timestamp(static_cast<uint64_t>(i) * 1000)
                     .f32(static_cast<float>(i))
                     .build());
  }

  auto parser = parseBuffer(b);
  auto& ts = parser->getTimeseriesMap().at("sensor");
  ASSERT_EQ(ts.timestamps.size(), 10u);
  ASSERT_EQ(ts.data[0].second.size(), 10u);
  for (int i = 0; i < 10; i++)
  {
    EXPECT_EQ(ts.timestamps[i].value(), static_cast<uint64_t>(i) * 1000);
    EXPECT_FLOAT_EQ(ts.data[0].second[i], static_cast<float>(i));
  }
}

TEST(ULogParserTest, MultipleFormatsAndSubscriptions)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("accel:uint64_t timestamp;float ax;float ay;");
  b.addFormat("gyro:uint64_t timestamp;float gx;");
  b.addSubscription(0, 0, "accel");
  b.addSubscription(1, 0, "gyro");
  b.addData(0, DataPayloadBuilder().timestamp(100).f32(1.0f).f32(2.0f).build());
  b.addData(1, DataPayloadBuilder().timestamp(200).f32(9.0f).build());
  b.addData(0, DataPayloadBuilder().timestamp(300).f32(3.0f).f32(4.0f).build());

  auto parser = parseBuffer(b);
  auto& ts_map = parser->getTimeseriesMap();
  ASSERT_EQ(ts_map.size(), 2u);

  auto& accel = ts_map.at("accel");
  ASSERT_EQ(accel.timestamps.size(), 2u);
  EXPECT_EQ(accel.timestamps[0].value(), 100u);
  EXPECT_EQ(accel.timestamps[1].value(), 300u);
  EXPECT_EQ(accel.data[0].first, "/ax");
  EXPECT_EQ(accel.data[1].first, "/ay");

  auto& gyro = ts_map.at("gyro");
  ASSERT_EQ(gyro.timestamps.size(), 1u);
  EXPECT_FLOAT_EQ(gyro.data[0].second[0], 9.0f);
}

// ===========================================================================
// Timestamp edge cases
// ===========================================================================

TEST(ULogParserTest, TimestampAtNonZeroIndex)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("t:float x;uint64_t timestamp;float y;");
  b.addSubscription(0, 0, "t");
  b.addData(0, DataPayloadBuilder().f32(1.5f).timestamp(5000).f32(2.5f).build());

  auto parser = parseBuffer(b);
  auto& ts = parser->getTimeseriesMap().at("t");
  ASSERT_EQ(ts.timestamps.size(), 1u);
  EXPECT_EQ(ts.timestamps[0].value(), 5000u);
  ASSERT_EQ(ts.data.size(), 2u);
  EXPECT_EQ(ts.data[0].first, "/x");
  EXPECT_FLOAT_EQ(ts.data[0].second[0], 1.5f);
  EXPECT_EQ(ts.data[1].first, "/y");
  EXPECT_FLOAT_EQ(ts.data[1].second[0], 2.5f);
}

TEST(ULogParserTest, ArrayFieldsBeforeTimestamp)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("t:float[3] v;uint64_t timestamp;float z;");
  b.addSubscription(0, 0, "t");
  b.addData(0,
            DataPayloadBuilder().f32(1.0f).f32(2.0f).f32(3.0f).timestamp(7000).f32(4.0f).build());

  auto parser = parseBuffer(b);
  auto& ts = parser->getTimeseriesMap().at("t");
  EXPECT_EQ(ts.timestamps[0].value(), 7000u);
  ASSERT_EQ(ts.data.size(), 4u);
  EXPECT_EQ(ts.data[0].first, "/v.00");
  EXPECT_EQ(ts.data[1].first, "/v.01");
  EXPECT_EQ(ts.data[2].first, "/v.02");
  EXPECT_EQ(ts.data[3].first, "/z");
  EXPECT_FLOAT_EQ(ts.data[0].second[0], 1.0f);
  EXPECT_FLOAT_EQ(ts.data[1].second[0], 2.0f);
  EXPECT_FLOAT_EQ(ts.data[2].second[0], 3.0f);
  EXPECT_FLOAT_EQ(ts.data[3].second[0], 4.0f);
}

// ===========================================================================
// No timestamp
// ===========================================================================

TEST(ULogParserTest, MessageWithoutTimestamp)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("status:uint8_t flags;float value;");
  b.addSubscription(0, 0, "status");
  b.addData(0, DataPayloadBuilder().u8(0x42).f32(9.81f).build());

  auto parser = parseBuffer(b);
  auto& ts = parser->getTimeseriesMap().at("status");
  ASSERT_EQ(ts.timestamps.size(), 1u);
  EXPECT_FALSE(ts.timestamps[0].has_value());
  ASSERT_EQ(ts.data.size(), 2u);
  EXPECT_DOUBLE_EQ(ts.data[0].second[0], 0x42);
  EXPECT_FLOAT_EQ(ts.data[1].second[0], 9.81f);
}

// ===========================================================================
// Nested types
// ===========================================================================

TEST(ULogParserTest, NestedType)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("vec3:float x;float y;float z;");
  b.addFormat("pose:uint64_t timestamp;vec3 pos;");
  b.addSubscription(0, 0, "pose");
  b.addData(0, DataPayloadBuilder().timestamp(1000).f32(1.0f).f32(2.0f).f32(3.0f).build());

  auto parser = parseBuffer(b);
  auto& ts = parser->getTimeseriesMap().at("pose");
  ASSERT_EQ(ts.data.size(), 3u);
  EXPECT_EQ(ts.data[0].first, "/pos/x");
  EXPECT_EQ(ts.data[1].first, "/pos/y");
  EXPECT_EQ(ts.data[2].first, "/pos/z");
  EXPECT_FLOAT_EQ(ts.data[0].second[0], 1.0f);
  EXPECT_FLOAT_EQ(ts.data[1].second[0], 2.0f);
  EXPECT_FLOAT_EQ(ts.data[2].second[0], 3.0f);
}

TEST(ULogParserTest, ArrayOfNestedType)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("esc_report:uint8_t id;float voltage;");
  b.addFormat("esc:uint64_t timestamp;esc_report[4] report;");
  b.addSubscription(0, 0, "esc");

  auto pb = DataPayloadBuilder();
  pb.timestamp(2000);
  for (int i = 0; i < 4; i++)
  {
    pb.u8(static_cast<uint8_t>(i));
    pb.f32(11.0f + static_cast<float>(i));
  }
  b.addData(0, pb.build());

  auto parser = parseBuffer(b);
  auto& ts = parser->getTimeseriesMap().at("esc");
  ASSERT_EQ(ts.data.size(), 8u);
  EXPECT_EQ(ts.data[0].first, "/report.00/id");
  EXPECT_EQ(ts.data[1].first, "/report.00/voltage");
  EXPECT_EQ(ts.data[2].first, "/report.01/id");
  EXPECT_EQ(ts.data[3].first, "/report.01/voltage");
  EXPECT_EQ(ts.data[6].first, "/report.03/id");
  EXPECT_EQ(ts.data[7].first, "/report.03/voltage");
  EXPECT_FLOAT_EQ(ts.data[1].second[0], 11.0f);
  EXPECT_FLOAT_EQ(ts.data[7].second[0], 14.0f);
}

// ===========================================================================
// Info messages
// ===========================================================================

TEST(ULogParserTest, InfoStringMessage)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addInfoString("sys_name", "PX4_FMU_V5");
  b.addFormat("s:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "s");

  auto parser = parseBuffer(b);
  auto& info = parser->getInfo();
  ASSERT_EQ(info.count("sys_name"), 1u);
  EXPECT_EQ(info.at("sys_name"), "PX4_FMU_V5");
}

TEST(ULogParserTest, InfoNumericTypes)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addInfoNumeric<uint32_t>("uint32_t", "board_rev", 42);
  b.addInfoNumeric<int32_t>("int32_t", "neg_val", -7);
  b.addInfoNumeric<float>("float", "pi", 3.14f);
  b.addInfoNumeric<uint64_t>("uint64_t", "big_num", 123456789ULL);
  b.addFormat("s:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "s");

  auto parser = parseBuffer(b);
  auto& info = parser->getInfo();
  EXPECT_EQ(info.at("board_rev"), "42");
  EXPECT_EQ(info.at("neg_val"), "-7");
  EXPECT_EQ(info.at("big_num"), "123456789");
  EXPECT_NE(info.find("pi"), info.end());
}

TEST(ULogParserTest, InfoVersionRelease)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addInfoNumeric<uint32_t>("uint32_t", "ver_hw_release", 0x00500003);
  b.addFormat("s:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "s");

  auto parser = parseBuffer(b);
  EXPECT_EQ(parser->getInfo().at("ver_hw_release"), "0x00500003");
}

// ===========================================================================
// Parameters
// ===========================================================================

TEST(ULogParserTest, ParameterFloat)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addParameterFloat("MC_ROLLRATE_P", 0.15f);
  b.addFormat("s:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "s");

  auto parser = parseBuffer(b);
  auto& params = parser->getParameters();
  ASSERT_EQ(params.size(), 1u);
  EXPECT_EQ(params[0].name, "MC_ROLLRATE_P");
  EXPECT_EQ(params[0].val_type, ULogParser::FLOAT);
  EXPECT_FLOAT_EQ(params[0].value.val_real, 0.15f);
}

TEST(ULogParserTest, ParameterInt)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addParameterInt("SYS_AUTOSTART", 4001);
  b.addFormat("s:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "s");

  auto parser = parseBuffer(b);
  auto& params = parser->getParameters();
  ASSERT_EQ(params.size(), 1u);
  EXPECT_EQ(params[0].name, "SYS_AUTOSTART");
  EXPECT_EQ(params[0].val_type, ULogParser::INT32);
  EXPECT_EQ(params[0].value.val_int, 4001);
}

TEST(ULogParserTest, ParameterOverrideInDataSection)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addParameterFloat("MY_PARAM", 1.0f);
  b.addFormat("s:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "s");
  // Data-section parameter overrides definition-section value
  b.addParameterFloat("MY_PARAM", 2.0f);

  auto parser = parseBuffer(b);
  auto& params = parser->getParameters();
  bool found = false;
  for (auto& p : params)
  {
    if (p.name == "MY_PARAM")
    {
      EXPECT_FLOAT_EQ(p.value.val_real, 2.0f);
      found = true;
    }
  }
  EXPECT_TRUE(found);
}

TEST(ULogParserTest, ParameterDefaultIgnored)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  float dummy = 99.0f;
  b.addParameterDefault("float", "IGNORED_PARAM", &dummy, sizeof(dummy));
  b.addFormat("s:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "s");

  auto parser = parseBuffer(b);
  for (auto& p : parser->getParameters())
  {
    EXPECT_NE(p.name, "IGNORED_PARAM");
  }
}

// ===========================================================================
// Multi-instance
// ===========================================================================

TEST(ULogParserTest, MultiIdSubscriptions)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("sensor:uint64_t timestamp;float val;");
  b.addSubscription(0, 0, "sensor");
  b.addSubscription(1, 1, "sensor");
  b.addData(0, DataPayloadBuilder().timestamp(100).f32(1.0f).build());
  b.addData(1, DataPayloadBuilder().timestamp(200).f32(2.0f).build());

  auto parser = parseBuffer(b);
  auto& ts_map = parser->getTimeseriesMap();
  // multi_id > 0 seen → all instances get ".XX" suffix
  ASSERT_EQ(ts_map.count("sensor.00"), 1u);
  ASSERT_EQ(ts_map.count("sensor.01"), 1u);
  EXPECT_FLOAT_EQ(ts_map.at("sensor.00").data[0].second[0], 1.0f);
  EXPECT_FLOAT_EQ(ts_map.at("sensor.01").data[0].second[0], 2.0f);
}

// ===========================================================================
// Data types
// ===========================================================================

TEST(ULogParserTest, AllPrimitiveTypes)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("all:uint64_t timestamp;uint8_t u8;uint16_t u16;uint32_t u32;"
              "uint64_t u64_val;int8_t i8;int16_t i16;int32_t i32;int64_t i64;"
              "float f;double d;bool b;char c;");
  b.addSubscription(0, 0, "all");
  b.addData(0, DataPayloadBuilder()
                   .timestamp(1000)
                   .u8(255)
                   .u16(1000)
                   .u32(100000)
                   .u64(9999999999ULL)
                   .i8(-42)
                   .i16(-1000)
                   .i32(-100000)
                   .i64(-9999999999LL)
                   .f32(3.14f)
                   .f64(2.71828)
                   .boolean(true)
                   .ch('Z')
                   .build());

  auto parser = parseBuffer(b);
  auto& ts = parser->getTimeseriesMap().at("all");
  ASSERT_EQ(ts.data.size(), 12u);

  size_t i = 0;
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], 255.0);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], 1000.0);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], 100000.0);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], 9999999999.0);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], -42.0);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], -1000.0);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], -100000.0);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], -9999999999.0);
  EXPECT_FLOAT_EQ(static_cast<float>(ts.data[i++].second[0]), 3.14f);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], 2.71828);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], 1.0);
  EXPECT_DOUBLE_EQ(ts.data[i++].second[0], static_cast<double>('Z'));
}

// ===========================================================================
// Padding
// ===========================================================================

TEST(ULogParserTest, PaddingFieldsSkipped)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("msg:uint64_t timestamp;uint8_t val;uint8_t[3] _padding0;float x;");
  b.addSubscription(0, 0, "msg");
  b.addData(0, DataPayloadBuilder().timestamp(1000).u8(42).padding(3).f32(1.5f).build());

  auto parser = parseBuffer(b);
  auto& ts = parser->getTimeseriesMap().at("msg");
  ASSERT_EQ(ts.data.size(), 2u);
  EXPECT_EQ(ts.data[0].first, "/val");
  EXPECT_EQ(ts.data[1].first, "/x");
  EXPECT_DOUBLE_EQ(ts.data[0].second[0], 42.0);
  EXPECT_FLOAT_EQ(ts.data[1].second[0], 1.5f);
}

// ===========================================================================
// Logging
// ===========================================================================

TEST(ULogParserTest, LoggingMessage)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("s:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "s");
  b.addLogging('4', 5000, "Test log message");

  auto parser = parseBuffer(b);
  auto& logs = parser->getLogs();
  ASSERT_EQ(logs.size(), 1u);
  EXPECT_EQ(logs[0].level, '4');
  EXPECT_EQ(logs[0].timestamp, 5000u);
  EXPECT_EQ(logs[0].msg, "Test log message");
}

// ===========================================================================
// Error handling
// ===========================================================================

TEST(ULogParserTest, InvalidMagicThrows)
{
  std::vector<uint8_t> buf(24, 0);
  EXPECT_THROW(ULogParser parser(buf.data(), buf.size()), std::runtime_error);
}

TEST(ULogParserTest, TruncatedAfterHeaderThrows)
{
  // Valid header but no definition messages follow
  ULogBufferBuilder b;
  // Pad with 3 bytes (msg header len) so the first read doesn't overrun
  std::vector<uint8_t> buf(b.size() + 3, 0);
  memcpy(buf.data(), b.data(), b.size());
  EXPECT_THROW(ULogParser parser(buf.data(), buf.size()), std::runtime_error);
}

TEST(ULogParserTest, EmptyDataSection)
{
  ULogBufferBuilder b;
  b.addFlagBits();
  b.addFormat("sensor:uint64_t timestamp;float x;");
  b.addSubscription(0, 0, "sensor");
  // No DATA messages

  auto parser = parseBuffer(b);
  EXPECT_TRUE(parser->getTimeseriesMap().empty());
}
