// tools/decoders/ros1_decoder.hpp
#pragma once
#include "decoders/decoder.hpp"
#include "decoders/msg_parser.hpp"

namespace mosaico {

class Ros1Decoder : public MessageDecoder {
 public:
  std::vector<std::string> supportedEncodings() const override;
  arrow::Status prepare(const DecoderContext& ctx) override;
  arrow::Status decode(const std::byte* data, uint64_t size,
                        const DecoderContext& ctx,
                        FieldMap& out) override;

 private:
  std::shared_ptr<MsgSchema> msg_schema_;
};

}  // namespace mosaico
