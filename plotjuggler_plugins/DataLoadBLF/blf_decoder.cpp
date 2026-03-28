#include "blf_decoder.h"

#include "blf_series_naming.h"

#include <algorithm>
#include <exception>

namespace PJ::BLF
{

BlfDecoderPipeline::BlfDecoderPipeline(const BlfPluginConfig& config, DbcManager* dbc_manager,
                                       ISeriesWriter* series_writer)
  : config_(config), dbc_manager_(dbc_manager), series_writer_(series_writer)
{
}

bool BlfDecoderPipeline::ProcessFrame(const NormalizedCanFrame& frame)
{
  ++stats_.frames_processed;
  if (!series_writer_)
  {
    ++stats_.decode_errors;
    return false;
  }

  const double timestamp = ResolveTimestamp(frame);

  if (config_.emit_raw)
  {
    EmitRaw(frame, timestamp);
  }

  if (config_.emit_decoded && dbc_manager_)
  {
    try
    {
      EmitDecoded(frame, timestamp);
    }
    catch (const std::exception&)
    {
      ++stats_.decode_errors;
      return false;
    }
  }

  return true;
}

BlfDecodeStats BlfDecoderPipeline::stats() const
{
  return stats_;
}

double BlfDecoderPipeline::ResolveTimestamp(const NormalizedCanFrame& frame) const
{
  if (config_.use_source_timestamp)
  {
    return frame.timestamp;
  }
  return static_cast<double>(stats_.frames_processed - 1U);
}

void BlfDecoderPipeline::EmitRaw(const NormalizedCanFrame& frame, double timestamp)
{
  const BlfFrameKey key{frame.channel, frame.id, frame.extended};
  const std::string prefix = RawSeriesPrefix(key);

  series_writer_->WriteSample(prefix + "/dlc", timestamp, static_cast<double>(frame.dlc));
  series_writer_->WriteSample(prefix + "/is_fd", timestamp, frame.is_fd ? 1.0 : 0.0);
  series_writer_->WriteSample(prefix + "/is_brs", timestamp, frame.is_brs ? 1.0 : 0.0);
  series_writer_->WriteSample(prefix + "/is_esi", timestamp, frame.is_esi ? 1.0 : 0.0);
  stats_.raw_samples_written += 4;

  const std::size_t payload_size =
      std::min<std::size_t>(frame.size, static_cast<std::size_t>(frame.data.size()));
  for (std::size_t i = 0; i < payload_size; ++i)
  {
    series_writer_->WriteSample(RawByteSeriesName(key, i), timestamp,
                                static_cast<double>(frame.data[i]));
    ++stats_.raw_samples_written;
  }
}

void BlfDecoderPipeline::EmitDecoded(const NormalizedCanFrame& frame, double timestamp)
{
  const std::size_t payload_size =
      std::min<std::size_t>(frame.size, static_cast<std::size_t>(frame.data.size()));
  const auto decoded_signals = dbc_manager_->DecodeForChannel(
      frame.channel, frame.id, frame.extended, frame.is_fd, frame.dlc, frame.data.data(),
      payload_size);

  for (const auto& signal : decoded_signals)
  {
    series_writer_->WriteSample(
        DecodedSeriesName(frame.channel, signal.message, signal.signal), timestamp, signal.value);
    ++stats_.decoded_samples_written;
  }
}

}  // namespace PJ::BLF
