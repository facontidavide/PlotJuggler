// tools/decoders/decoder.hpp
#pragma once

#include "ontology/field_map.hpp"

#include <arrow/status.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mosaico {

struct DecoderContext {
  std::string schema_name;
  std::string schema_encoding;
  std::vector<std::byte> schema_data;

  struct Sample {
    uint64_t log_time;
    const std::byte* data;
    uint64_t size;
  };
  std::vector<Sample> samples;
};

class MessageDecoder {
 public:
  virtual ~MessageDecoder() = default;

  virtual std::vector<std::string> supportedEncodings() const = 0;

  virtual bool needsSamples() const { return false; }

  /// Prepare the decoder using the given context (schema data, samples, etc.).
  /// Not all decoders need this; default is a no-op.
  virtual arrow::Status prepare(const DecoderContext& /*ctx*/) {
    return arrow::Status::OK();
  }

  virtual arrow::Status decode(const std::byte* data, uint64_t size,
                                const DecoderContext& ctx,
                                FieldMap& out) = 0;
};

}  // namespace mosaico
