#pragma once

#include <cstdint>
#include <vector>

#include "rosx_introspection/message_writer.hpp"

namespace RosMsgParser {

/// MessageWriter that produces MessagePack directly during the schema walk,
/// bypassing FlatMessage entirely.
class MsgpackMessageWriter : public MessageWriter {
 public:
  explicit MsgpackMessageWriter(std::vector<uint8_t>* output);

  void writeValue(const FieldLeaf& leaf, const Variant& value) override;
  void writeString(const FieldLeaf& leaf, const std::string& value) override;
  void writeEnum(const FieldLeaf& leaf, int32_t int_value, const std::string& enum_name) override;
  void finish() override;

 private:
  void ensureCapacity(size_t additional);
  void writeKey(const FieldLeaf& leaf);

  std::vector<uint8_t>* _output;
  size_t _offset = 0;
  uint32_t _count = 0;
  std::string _key_buf;
};

}  // namespace RosMsgParser
