// tools/decoders/protobuf_decoder.hpp
#pragma once
#include "decoders/decoder.hpp"

#include <memory>

namespace google::protobuf {
class DescriptorPool;
class DynamicMessageFactory;
class Descriptor;
class FileDescriptorSet;
}  // namespace google::protobuf

namespace mosaico {

class ProtobufDecoder : public MessageDecoder {
 public:
  ProtobufDecoder();
  ~ProtobufDecoder() override;

  std::vector<std::string> supportedEncodings() const override;
  arrow::Status prepare(const DecoderContext& ctx) override;
  arrow::Status decode(const std::byte* data, uint64_t size,
                        const DecoderContext& ctx,
                        FieldMap& out) override;

 private:
  std::unique_ptr<google::protobuf::DescriptorPool> pool_;
  std::unique_ptr<google::protobuf::DynamicMessageFactory> factory_;
  const google::protobuf::Descriptor* message_desc_ = nullptr;
};

}  // namespace mosaico
