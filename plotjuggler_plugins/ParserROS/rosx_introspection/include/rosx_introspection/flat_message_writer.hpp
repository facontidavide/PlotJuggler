#pragma once

#include <memory>

#include "rosx_introspection/message_writer.hpp"

namespace RosMsgParser {

struct FlatMessage;
class Parser;

class FlatMessageWriter : public MessageWriter {
 public:
  FlatMessageWriter(FlatMessage* flat, int blob_policy);
  ~FlatMessageWriter() override;

  FlatMessageWriter(const FlatMessageWriter&) = delete;
  FlatMessageWriter& operator=(const FlatMessageWriter&) = delete;

  void writeValue(const FieldLeaf& leaf, const Variant& value) override;
  void writeString(const FieldLeaf& leaf, const std::string& str) override;
  void writeEnum(const FieldLeaf& leaf, int32_t value, const std::string& name) override;
  void writeBlob(const FieldLeaf& leaf, Span<const uint8_t> data) override;
  void finish() override;

 private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};

}  // namespace RosMsgParser
