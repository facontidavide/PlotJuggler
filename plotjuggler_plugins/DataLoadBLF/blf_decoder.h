#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "blf_config.h"
#include "dbc_manager.h"

namespace PJ::BLF
{

struct NormalizedCanFrame
{
  double timestamp = 0.0;
  uint32_t channel = 0;
  uint32_t id = 0;
  bool is_fd = false;
  bool is_brs = false;
  bool is_esi = false;
  bool extended = false;
  uint8_t dlc = 0;
  uint8_t size = 0;
  std::array<uint8_t, 64> data{};
};

struct BlfDecodeStats
{
  uint64_t frames_processed = 0;
  uint64_t raw_samples_written = 0;
  uint64_t decoded_samples_written = 0;
  uint64_t decode_errors = 0;
};

class ISeriesWriter
{
public:
  virtual ~ISeriesWriter() = default;
  virtual void WriteSample(const std::string& series_name, double timestamp, double value) = 0;
};

class BlfDecoderPipeline
{
public:
  BlfDecoderPipeline(const BlfPluginConfig& config, DbcManager* dbc_manager,
                     ISeriesWriter* series_writer);

  bool ProcessFrame(const NormalizedCanFrame& frame);
  BlfDecodeStats stats() const;

private:
  double ResolveTimestamp(const NormalizedCanFrame& frame) const;
  void EmitRaw(const NormalizedCanFrame& frame, double timestamp);
  bool EmitDecoded(const NormalizedCanFrame& frame, double timestamp);

  BlfPluginConfig config_;
  DbcManager* dbc_manager_ = nullptr;
  ISeriesWriter* series_writer_ = nullptr;
  BlfDecodeStats stats_;
};

}  // namespace PJ::BLF
