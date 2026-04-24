#pragma once

#include "rosx_introspection/stringtree_leaf.hpp"

namespace RosMsgParser {

class ROSField;

/// Abstract interface for consuming deserialized schema values.
/// Implement this to produce different output formats (FlatMessage, JSON, msgpack, etc.)
/// from the same schema walk.
class MessageWriter {
 public:
  virtual ~MessageWriter() = default;

  /// Called for each scalar/builtin value (not string, not enum)
  virtual void writeValue(const FieldLeaf& leaf, const Variant& value) = 0;

  /// Called for each string value
  virtual void writeString(const FieldLeaf& leaf, const std::string& value) = 0;

  /// Called for each enum value
  virtual void writeEnum(const FieldLeaf& leaf, int32_t int_value, const std::string& enum_name) = 0;

  /// Called for blob data (large byte arrays exceeding max_array_size)
  virtual void writeBlob(const FieldLeaf& /*leaf*/, Span<const uint8_t> /*data*/) {}

  /// Structural events for writers that need hierarchy (e.g., JSON)
  virtual void beginStruct(const ROSField& /*field*/) {}
  virtual void endStruct() {}

  /// Called when the schema walk finishes. Writers can use this to finalize output.
  virtual void finish() {}
};

}  // namespace RosMsgParser
