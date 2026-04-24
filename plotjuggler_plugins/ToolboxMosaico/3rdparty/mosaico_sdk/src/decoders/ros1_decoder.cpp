// tools/decoders/ros1_decoder.cpp
#include "decoders/ros1_decoder.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

namespace mosaico {
namespace {

// ---------------------------------------------------------------------------
// Simple sequential buffer reader -- no alignment, little-endian.
// ROS1 serialization packs bytes with no padding and no encapsulation header.
// All reads are bounds-checked: a truncated or crafted payload throws a
// std::runtime_error that Ros1Decoder::decode() catches and converts into
// arrow::Status::Invalid, rather than triggering an out-of-bounds memcpy
// (which on a mmap-backed MCAP can cross into unmapped memory and SIGSEGV).
// ---------------------------------------------------------------------------
class RosReader {
 public:
  RosReader(const std::byte* data, uint64_t size)
      : data_(data), size_(size), pos_(0) {}

  template <typename T>
  T read() {
    if (pos_ + sizeof(T) > size_) {
      throw std::runtime_error("ROS1 read past end of buffer");
    }
    T val{};
    std::memcpy(&val, data_ + pos_, sizeof(T));
    pos_ += sizeof(T);
    return val;
  }

  // ROS1 string: uint32 length, then `length` UTF-8 bytes (no null terminator).
  // The length is attacker-influenced; a value of, say, 0xFFFFFF00 on a tiny
  // buffer must not translate into a 4 GiB std::string constructor call.
  std::string readString() {
    uint32_t len = read<uint32_t>();
    if (pos_ + len > size_) {
      throw std::runtime_error("ROS1 string length exceeds buffer");
    }
    std::string s(reinterpret_cast<const char*>(data_ + pos_), len);
    pos_ += len;
    return s;
  }

  uint64_t remaining() const { return size_ - pos_; }

 private:
  const std::byte* data_;
  uint64_t size_;
  uint64_t pos_;
};

// ---------------------------------------------------------------------------
// Read a single primitive from ROS1 and return it as a FieldValue.
// ---------------------------------------------------------------------------
FieldValue readPrimitive(RosReader& reader, PrimitiveType prim) {
  switch (prim) {
    case PrimitiveType::kBool:    return reader.read<bool>();
    case PrimitiveType::kInt8:    return reader.read<int8_t>();
    case PrimitiveType::kUint8:   return reader.read<uint8_t>();
    case PrimitiveType::kInt16:   return reader.read<int16_t>();
    case PrimitiveType::kUint16:  return reader.read<uint16_t>();
    case PrimitiveType::kInt32:   return reader.read<int32_t>();
    case PrimitiveType::kUint32:  return reader.read<uint32_t>();
    case PrimitiveType::kInt64:   return reader.read<int64_t>();
    case PrimitiveType::kUint64:  return reader.read<uint64_t>();
    case PrimitiveType::kFloat32: return reader.read<float>();
    case PrimitiveType::kFloat64: return reader.read<double>();
    case PrimitiveType::kString:
    case PrimitiveType::kWstring: return reader.readString();
  }
  return std::string("<unknown>");
}

// Forward declaration.
arrow::Status decodeField(RosReader& reader,
                          const MsgField& field,
                          const MsgSchema& parent_schema,
                          const std::string& prefix,
                          FieldMap& out);

// ---------------------------------------------------------------------------
// Decode all fields of a nested struct into the FieldMap with dot-path prefix.
// ---------------------------------------------------------------------------
arrow::Status decodeStructFields(RosReader& reader,
                                 const MsgSchema& schema,
                                 const std::string& prefix,
                                 FieldMap& out) {
  for (const auto& field : schema.fields) {
    ARROW_RETURN_NOT_OK(decodeField(reader, field, schema, prefix, out));
  }
  return arrow::Status::OK();
}

// ---------------------------------------------------------------------------
// Decode a single element (for use in arrays).
// ---------------------------------------------------------------------------
arrow::Status decodeArrayElement(RosReader& reader,
                                 const MsgField& field,
                                 const MsgSchema& parent_schema,
                                 FieldMap& element_out) {
  if (field.primitive.has_value()) {
    element_out.fields["value"] = readPrimitive(reader, *field.primitive);
    return arrow::Status::OK();
  }
  auto it = parent_schema.nested.find(field.nested_type);
  if (it == parent_schema.nested.end()) {
    return arrow::Status::Invalid("Unresolved nested type: " +
                                  field.nested_type);
  }
  return decodeStructFields(reader, *it->second, "", element_out);
}

// ---------------------------------------------------------------------------
// Decode a single field (scalar, fixed-array, or dynamic-array).
// ---------------------------------------------------------------------------
arrow::Status decodeField(RosReader& reader,
                          const MsgField& field,
                          const MsgSchema& parent_schema,
                          const std::string& prefix,
                          FieldMap& out) {
  std::string key = prefix.empty() ? field.name : prefix + "." + field.name;

  if (field.array_size == 0) {
    // Scalar
    if (field.primitive.has_value()) {
      out.fields[key] = readPrimitive(reader, *field.primitive);
      return arrow::Status::OK();
    }
    // Nested struct -- flatten into dot-path keys
    auto it = parent_schema.nested.find(field.nested_type);
    if (it == parent_schema.nested.end()) {
      return arrow::Status::Invalid("Unresolved nested type: " +
                                    field.nested_type);
    }
    return decodeStructFields(reader, *it->second, key, out);
  }

  // Array (fixed or dynamic)
  int32_t count = field.array_size;
  if (count < 0) {
    // Dynamic array -- read uint32 length prefix
    count = static_cast<int32_t>(reader.read<uint32_t>());
  }

  // For primitive arrays, produce appropriate vector types
  if (field.primitive.has_value()) {
    // uint8 arrays -> vector<uint8_t>
    if (*field.primitive == PrimitiveType::kUint8) {
      std::vector<uint8_t> vals;
      vals.reserve(count);
      for (int32_t i = 0; i < count; ++i) {
        vals.push_back(reader.read<uint8_t>());
      }
      out.fields[key] = std::move(vals);
      return arrow::Status::OK();
    }
    // All other numeric primitives -> vector<double>
    std::vector<double> vals;
    vals.reserve(count);
    for (int32_t i = 0; i < count; ++i) {
      auto fv = readPrimitive(reader, *field.primitive);
      std::visit([&](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_arithmetic_v<T>) {
          vals.push_back(static_cast<double>(v));
        }
      }, fv);
    }
    out.fields[key] = std::move(vals);
    return arrow::Status::OK();
  }

  // Array of nested structs -> vector<FieldMap>
  MsgField elem_field;
  elem_field.name = field.name;
  elem_field.primitive = field.primitive;
  elem_field.nested_type = field.nested_type;
  elem_field.array_size = 0;

  std::vector<FieldMap> elements;
  elements.reserve(count);
  for (int32_t i = 0; i < count; ++i) {
    FieldMap elem;
    ARROW_RETURN_NOT_OK(
        decodeArrayElement(reader, elem_field, parent_schema, elem));
    elements.push_back(std::move(elem));
  }
  out.fields[key] = std::move(elements);
  return arrow::Status::OK();
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::vector<std::string> Ros1Decoder::supportedEncodings() const {
  return {"ros1"};
}

arrow::Status Ros1Decoder::prepare(const DecoderContext& ctx) {
  std::string text(reinterpret_cast<const char*>(ctx.schema_data.data()),
                   ctx.schema_data.size());

  msg_schema_ = parseMsgDef(ctx.schema_name, text);
  if (!msg_schema_) {
    return arrow::Status::Invalid(
        "Failed to parse .msg definition for " + ctx.schema_name);
  }
  return arrow::Status::OK();
}

arrow::Status Ros1Decoder::decode(
    const std::byte* data, uint64_t size,
    const DecoderContext& /*ctx*/,
    FieldMap& out) {
  if (!msg_schema_) {
    return arrow::Status::Invalid("prepare() must be called first");
  }

  // RosReader throws on any bounds violation; keep the cost of a corrupted
  // message to a per-message Status rather than letting it take the whole
  // upload down.
  try {
    RosReader reader(data, size);
    for (const auto& field : msg_schema_->fields) {
      ARROW_RETURN_NOT_OK(
          decodeField(reader, field, *msg_schema_, "", out));
    }
  } catch (const std::exception& e) {
    return arrow::Status::Invalid("ROS1 decode error: ", e.what());
  }

  return arrow::Status::OK();
}

}  // namespace mosaico
