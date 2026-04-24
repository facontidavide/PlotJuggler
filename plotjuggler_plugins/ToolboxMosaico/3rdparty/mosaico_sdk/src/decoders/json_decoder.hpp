// tools/decoders/json_decoder.hpp
#pragma once
#include "decoders/decoder.hpp"

namespace mosaico {

class JsonDecoder : public MessageDecoder {
 public:
  std::vector<std::string> supportedEncodings() const override;
  bool needsSamples() const override { return true; }
  arrow::Status decode(const std::byte* data, uint64_t size,
                        const DecoderContext& ctx,
                        FieldMap& out) override;
};

}  // namespace mosaico
