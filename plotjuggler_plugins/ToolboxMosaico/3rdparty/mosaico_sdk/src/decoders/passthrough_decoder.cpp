// tools/decoders/passthrough_decoder.cpp
#include "decoders/passthrough_decoder.hpp"

namespace mosaico {

std::vector<std::string> PassthroughDecoder::supportedEncodings() const {
  return {"flatbuffer", "cbor"};
}

arrow::Status PassthroughDecoder::decode(
    const std::byte* data, uint64_t size,
    const DecoderContext& /*ctx*/,
    FieldMap& out) {
  // Copy the raw bytes into a vector<uint8_t>.
  std::vector<uint8_t> raw(size);
  std::memcpy(raw.data(), data, size);
  out.fields["data"] = std::move(raw);
  return arrow::Status::OK();
}

}  // namespace mosaico
