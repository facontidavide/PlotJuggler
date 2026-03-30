#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "blf_config.h"
#include "blf_decoder.h"
#include "dbc_manager.h"

namespace PJ::BLF
{
namespace
{

struct WrittenSample
{
  std::string name;
  double timestamp = 0.0;
  double value = 0.0;
};

class RecordingSeriesWriter : public ISeriesWriter
{
public:
  void WriteSample(const std::string& series_name, double timestamp, double value) override
  {
    samples.push_back({series_name, timestamp, value});
  }

  bool HasSample(const std::string& series_name, double timestamp, double value) const
  {
    for (const auto& sample : samples)
    {
      if (sample.name == series_name && std::fabs(sample.timestamp - timestamp) < 1e-12 &&
          std::fabs(sample.value - value) < 1e-12)
      {
        return true;
      }
    }
    return false;
  }

  std::vector<WrittenSample> samples;
};

class FakeDecoder : public IDbcDecoder
{
public:
  std::vector<DecodedSignalValue> Decode(uint32_t can_id, bool extended, bool is_fd, uint8_t dlc,
                                         const uint8_t* data, std::size_t size) const override
  {
    (void)extended;
    (void)is_fd;
    (void)dlc;
    (void)data;
    (void)size;
    if (can_id != 0x123U)
    {
      return {};
    }
    return {DecodedSignalValue{"VehicleStatus", "SpeedKph", 88.5}};
  }
};

}  // namespace

TEST(BlfDecoderPipeline, WritesRawAndDecodedSeries)
{
  DbcManager manager([](const std::string&) { return std::make_unique<FakeDecoder>(); });
  ASSERT_TRUE(manager.LoadBindings({{7U, "vehicle.dbc"}}));

  BlfPluginConfig config;
  config.emit_raw = true;
  config.emit_decoded = true;
  config.use_source_timestamp = true;

  RecordingSeriesWriter writer;
  BlfDecoderPipeline pipeline(config, &manager, &writer);

  NormalizedCanFrame frame;
  frame.timestamp = 12.5;
  frame.channel = 7U;
  frame.id = 0x123U;
  frame.is_fd = true;
  frame.is_brs = true;
  frame.is_esi = false;
  frame.extended = false;
  frame.dlc = 8U;
  frame.size = 2U;
  frame.data[0] = 0x11U;
  frame.data[1] = 0x22U;

  ASSERT_TRUE(pipeline.ProcessFrame(frame));

  EXPECT_TRUE(writer.HasSample("raw/can7/0x123/dlc", 12.5, 8.0));
  EXPECT_TRUE(writer.HasSample("raw/can7/0x123/is_fd", 12.5, 1.0));
  EXPECT_TRUE(writer.HasSample("raw/can7/0x123/is_brs", 12.5, 1.0));
  EXPECT_TRUE(writer.HasSample("raw/can7/0x123/is_esi", 12.5, 0.0));
  EXPECT_TRUE(writer.HasSample("raw/can7/0x123/data_00", 12.5, 17.0));
  EXPECT_TRUE(writer.HasSample("raw/can7/0x123/data_01", 12.5, 34.0));
  EXPECT_TRUE(writer.HasSample("dbc/can7/VehicleStatus/SpeedKph", 12.5, 88.5));

  const BlfDecodeStats stats = pipeline.stats();
  EXPECT_EQ(stats.frames_processed, 1U);
  EXPECT_EQ(stats.raw_samples_written, 6U);
  EXPECT_EQ(stats.decoded_samples_written, 1U);
  EXPECT_EQ(stats.decode_errors, 0U);
}

TEST(BlfDecoderPipeline, FallsBackToRawWhenDbcCannotDecodeFrame)
{
  DbcManager manager([](const std::string&) { return std::make_unique<FakeDecoder>(); });
  ASSERT_TRUE(manager.LoadBindings({{1U, "vehicle.dbc"}}));

  BlfPluginConfig config;
  config.emit_raw = false;
  config.emit_decoded = true;
  config.use_source_timestamp = true;

  RecordingSeriesWriter writer;
  BlfDecoderPipeline pipeline(config, &manager, &writer);

  NormalizedCanFrame frame;
  frame.timestamp = 1.0;
  frame.channel = 1U;
  frame.id = 0x321U;  // FakeDecoder only decodes 0x123, this one should fallback to raw.
  frame.is_fd = false;
  frame.is_brs = false;
  frame.is_esi = false;
  frame.extended = false;
  frame.dlc = 8U;
  frame.size = 2U;
  frame.data[0] = 0xAAU;
  frame.data[1] = 0x55U;

  ASSERT_TRUE(pipeline.ProcessFrame(frame));

  EXPECT_TRUE(writer.HasSample("raw/can1/0x321/dlc", 1.0, 8.0));
  EXPECT_TRUE(writer.HasSample("raw/can1/0x321/data_00", 1.0, 170.0));
  EXPECT_TRUE(writer.HasSample("raw/can1/0x321/data_01", 1.0, 85.0));
  EXPECT_FALSE(writer.HasSample("dbc/can1/VehicleStatus/SpeedKph", 1.0, 88.5));

  const BlfDecodeStats stats = pipeline.stats();
  EXPECT_EQ(stats.frames_processed, 1U);
  EXPECT_EQ(stats.raw_samples_written, 6U);
  EXPECT_EQ(stats.decoded_samples_written, 0U);
  EXPECT_EQ(stats.decode_errors, 0U);
}

}  // namespace PJ::BLF
