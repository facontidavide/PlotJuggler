// tools/decoders/passthrough_decoder.hpp
#pragma once
#include "decoders/decoder.hpp"

namespace mosaico {

class PassthroughDecoder : public MessageDecoder {
 public:
  std::vector<std::string> supportedEncodings() const override;
  arrow::Status decode(const std::byte* data, uint64_t size,
                        const DecoderContext& ctx,
                        FieldMap& out) override;
};

}  // namespace mosaico
