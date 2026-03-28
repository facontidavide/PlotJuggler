#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "blf_decoder.h"
#include "blf_reader.h"
#include "dbcppp_decoder.h"

namespace PJ::BLF
{

class CountingWriter : public ISeriesWriter
{
public:
  void WriteSample(const std::string& series_name, double timestamp, double value) override
  {
    (void)timestamp;
    (void)value;
    ++samples_total;
    if (series_name.rfind("dbc/", 0) == 0)
    {
      ++decoded_series_samples;
    }
    else if (series_name.rfind("raw/", 0) == 0)
    {
      ++raw_series_samples;
    }
  }

  uint64_t samples_total = 0;
  uint64_t raw_series_samples = 0;
  uint64_t decoded_series_samples = 0;
};

}  // namespace PJ::BLF

int main(int argc, char** argv)
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " <input.blf> <input.dbc>\n";
    return 2;
  }

  const std::string blf_path = argv[1];
  const std::string dbc_path = argv[2];

  PJ::BLF::DbcManager dbc_manager([](const std::string& path) {
    std::string error;
    auto decoder = std::make_unique<PJ::BLF::DbcpppDecoder>(path, &error);
    if (!decoder->IsValid())
    {
      return std::unique_ptr<PJ::BLF::IDbcDecoder>();
    }
    return std::unique_ptr<PJ::BLF::IDbcDecoder>(std::move(decoder));
  });

  std::vector<PJ::BLF::ChannelDbcBinding> bindings;
  for (uint32_t channel = 0; channel <= 16; ++channel)
  {
    bindings.push_back({channel, dbc_path});
  }

  std::string load_error;
  if (!dbc_manager.LoadBindings(bindings, &load_error))
  {
    std::cerr << "Failed to load DBC bindings: " << load_error << "\n";
    return 3;
  }

  PJ::BLF::BlfPluginConfig config;
  config.emit_raw = true;
  config.emit_decoded = true;
  config.use_source_timestamp = true;

  PJ::BLF::CountingWriter writer;
  PJ::BLF::BlfDecoderPipeline pipeline(config, &dbc_manager, &writer);
  PJ::BLF::BlfReader reader;

  QString read_error;
  const bool ok = reader.ReadFrames(QString::fromStdString(blf_path),
                                    [&](const PJ::BLF::NormalizedCanFrame& frame) {
                                      pipeline.ProcessFrame(frame);
                                    },
                                    read_error);
  if (!ok)
  {
    std::cerr << "ReadFrames failed: " << read_error.toStdString() << "\n";
    return 4;
  }

  const auto stats = pipeline.stats();
  std::cout << "frames_processed=" << stats.frames_processed << "\n";
  std::cout << "raw_samples_written=" << stats.raw_samples_written << "\n";
  std::cout << "decoded_samples_written=" << stats.decoded_samples_written << "\n";
  std::cout << "decode_errors=" << stats.decode_errors << "\n";
  std::cout << "writer_samples_total=" << writer.samples_total << "\n";
  std::cout << "writer_raw_samples=" << writer.raw_series_samples << "\n";
  std::cout << "writer_decoded_samples=" << writer.decoded_series_samples << "\n";

  if (stats.frames_processed == 0 || stats.raw_samples_written == 0)
  {
    return 5;
  }

  return 0;
}
