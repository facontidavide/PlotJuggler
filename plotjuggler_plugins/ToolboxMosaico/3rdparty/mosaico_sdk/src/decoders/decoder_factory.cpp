// tools/decoders/decoder_factory.cpp
#include "decoders/decoder_factory.hpp"
#include "decoders/json_decoder.hpp"
#include "decoders/passthrough_decoder.hpp"

// Optional decoders — included only when enabled via CMake.
#ifdef MOSAICO_HAS_CDR
#include "decoders/cdr_decoder.hpp"
#endif
#ifdef MOSAICO_HAS_ROS1
#include "decoders/ros1_decoder.hpp"
#endif
#ifdef MOSAICO_HAS_PROTOBUF
#include "decoders/protobuf_decoder.hpp"
#endif

namespace mosaico {

std::unique_ptr<MessageDecoder> createDecoder(const std::string& encoding) {
  if (encoding == "json") return std::make_unique<JsonDecoder>();
#ifdef MOSAICO_HAS_CDR
  if (encoding == "cdr") return std::make_unique<CdrDecoder>();
#endif
#ifdef MOSAICO_HAS_ROS1
  if (encoding == "ros1") return std::make_unique<Ros1Decoder>();
#endif
#ifdef MOSAICO_HAS_PROTOBUF
  if (encoding == "protobuf") return std::make_unique<ProtobufDecoder>();
#endif
  if (encoding == "flatbuffer" || encoding == "cbor")
    return std::make_unique<PassthroughDecoder>();
  return nullptr;
}

}  // namespace mosaico
