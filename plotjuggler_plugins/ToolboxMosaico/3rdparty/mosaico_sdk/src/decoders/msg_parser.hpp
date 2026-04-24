// tools/decoders/msg_parser.hpp
#pragma once

#include <arrow/type_fwd.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mosaico {

enum class PrimitiveType {
  kBool,
  kInt8,
  kUint8,
  kInt16,
  kUint16,
  kInt32,
  kUint32,
  kInt64,
  kUint64,
  kFloat32,
  kFloat64,
  kString,
  kWstring,
};

struct MsgField {
  std::string name;

  // Exactly one of these is set:
  std::optional<PrimitiveType> primitive;  // leaf field
  std::string nested_type;                 // resolved later to a MsgSchema

  // Array info (0 = not an array, >0 = fixed-size, -1 = dynamic)
  int32_t array_size = 0;

  // Default value (from .msg "type name default_value" syntax). Ignored for now.
  std::string default_value;
};

struct MsgSchema {
  std::string full_name;  // e.g. "sensor_msgs/msg/Imu"
  std::vector<MsgField> fields;

  // Resolved nested schemas (populated by resolve()).
  std::unordered_map<std::string, std::shared_ptr<MsgSchema>> nested;
};

// Parse a complete .msg definition text (with === separators) into a resolved
// MsgSchema tree. Returns nullptr on parse error.
std::shared_ptr<MsgSchema> parseMsgDef(const std::string& schema_name,
                                        const std::string& text);

// Convert a resolved MsgSchema to an Arrow schema.
// Prepends a timestamp_ns (int64) field at position 0.
std::shared_ptr<arrow::Schema> msgSchemaToArrow(const MsgSchema& schema);

}  // namespace mosaico
