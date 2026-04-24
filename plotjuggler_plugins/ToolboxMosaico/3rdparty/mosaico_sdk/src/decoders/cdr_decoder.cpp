// tools/decoders/cdr_decoder.cpp
//
// CDR decoder backed by nanocdr (header-only, vendored via PlotJuggler's
// ParserROS plugin). Originally used eProsima fastcdr; switched to nanocdr
// to drop the libfastcdr-dev runtime dependency and align with the rest of
// the PlotJuggler ROS stack.
#include "decoders/cdr_decoder.hpp"

#include "nanocdr.hpp"

#include <stdexcept>
#include <string>

namespace mosaico {
namespace {

// ---------------------------------------------------------------------------
// Read a single primitive from CDR and return it as a FieldValue.
// ---------------------------------------------------------------------------
FieldValue readPrimitive(nanocdr::Decoder& cdr, PrimitiveType prim) {
  switch (prim) {
    case PrimitiveType::kBool: {
      bool v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kInt8: {
      int8_t v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kUint8: {
      uint8_t v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kInt16: {
      int16_t v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kUint16: {
      uint16_t v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kInt32: {
      int32_t v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kUint32: {
      uint32_t v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kInt64: {
      int64_t v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kUint64: {
      uint64_t v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kFloat32: {
      float v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kFloat64: {
      double v{}; cdr.decode(v); return v;
    }
    case PrimitiveType::kString:
    case PrimitiveType::kWstring: {
      std::string v; cdr.decode(v); return v;
    }
  }
  return std::string("<unknown>");
}

// Forward declaration.
arrow::Status decodeField(nanocdr::Decoder& cdr,
                          const MsgField& field,
                          const MsgSchema& parent_schema,
                          const std::string& prefix,
                          FieldMap& out);

// ---------------------------------------------------------------------------
// Decode all fields of a nested struct into the FieldMap with dot-path prefix.
// ---------------------------------------------------------------------------
arrow::Status decodeStructFields(nanocdr::Decoder& cdr,
                                 const MsgSchema& schema,
                                 const std::string& prefix,
                                 FieldMap& out) {
  for (const auto& field : schema.fields) {
    ARROW_RETURN_NOT_OK(decodeField(cdr, field, schema, prefix, out));
  }
  return arrow::Status::OK();
}

// ---------------------------------------------------------------------------
// Decode a single scalar element (for use in arrays).
// Returns a FieldMap for nested structs, or sets fields directly for primitives.
// ---------------------------------------------------------------------------
arrow::Status decodeArrayElement(nanocdr::Decoder& cdr,
                                 const MsgField& field,
                                 const MsgSchema& parent_schema,
                                 FieldMap& element_out) {
  if (field.primitive.has_value()) {
    // For array elements that are primitives, store with key "value"
    element_out.fields["value"] = readPrimitive(cdr, *field.primitive);
    return arrow::Status::OK();
  }
  // Nested struct element
  auto it = parent_schema.nested.find(field.nested_type);
  if (it == parent_schema.nested.end()) {
    return arrow::Status::Invalid("Unresolved nested type: " +
                                  field.nested_type);
  }
  return decodeStructFields(cdr, *it->second, "", element_out);
}

// ---------------------------------------------------------------------------
// Decode a single field (scalar, fixed-array, or dynamic-array).
// ---------------------------------------------------------------------------
arrow::Status decodeField(nanocdr::Decoder& cdr,
                          const MsgField& field,
                          const MsgSchema& parent_schema,
                          const std::string& prefix,
                          FieldMap& out) {
  std::string key = prefix.empty() ? field.name : prefix + "." + field.name;

  if (field.array_size == 0) {
    // Scalar
    if (field.primitive.has_value()) {
      out.fields[key] = readPrimitive(cdr, *field.primitive);
      return arrow::Status::OK();
    }
    // Nested struct — flatten into dot-path keys
    auto it = parent_schema.nested.find(field.nested_type);
    if (it == parent_schema.nested.end()) {
      return arrow::Status::Invalid("Unresolved nested type: " +
                                    field.nested_type);
    }
    return decodeStructFields(cdr, *it->second, key, out);
  }

  // Array (fixed or dynamic)
  int32_t count = field.array_size;
  if (count < 0) {
    // Dynamic array — read length from CDR
    uint32_t len = 0;
    cdr.decode(len);
    count = static_cast<int32_t>(len);
  }

  // For primitive arrays, produce a vector<double> or vector<uint8_t>
  if (field.primitive.has_value()) {
    // uint8 arrays -> vector<uint8_t>
    if (*field.primitive == PrimitiveType::kUint8) {
      std::vector<uint8_t> vals;
      vals.reserve(count);
      for (int32_t i = 0; i < count; ++i) {
        uint8_t v{}; cdr.decode(v);
        vals.push_back(v);
      }
      out.fields[key] = std::move(vals);
      return arrow::Status::OK();
    }
    // All other numeric primitives -> vector<double>
    std::vector<double> vals;
    vals.reserve(count);
    for (int32_t i = 0; i < count; ++i) {
      auto fv = readPrimitive(cdr, *field.primitive);
      // Convert the FieldValue to double
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
        decodeArrayElement(cdr, elem_field, parent_schema, elem));
    elements.push_back(std::move(elem));
  }
  out.fields[key] = std::move(elements);
  return arrow::Status::OK();
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::vector<std::string> CdrDecoder::supportedEncodings() const {
  return {"cdr"};
}

arrow::Status CdrDecoder::prepare(const DecoderContext& ctx) {
  std::string text(reinterpret_cast<const char*>(ctx.schema_data.data()),
                   ctx.schema_data.size());

  msg_schema_ = parseMsgDef(ctx.schema_name, text);
  if (!msg_schema_) {
    return arrow::Status::Invalid("Failed to parse .msg definition for " +
                                  ctx.schema_name);
  }
  return arrow::Status::OK();
}

arrow::Status CdrDecoder::decode(
    const std::byte* data, uint64_t size,
    const DecoderContext& /*ctx*/,
    FieldMap& out) {
  if (size < 4) {
    return arrow::Status::Invalid("CDR message too short (< 4 bytes)");
  }
  if (!msg_schema_) {
    return arrow::Status::Invalid("prepare() must be called first");
  }

  // nanocdr::Decoder reads the 4-byte encapsulation header itself and
  // honors the endianness flag. Give it the whole payload.
  try {
    nanocdr::ConstBuffer buffer(
        reinterpret_cast<const uint8_t*>(data),
        static_cast<size_t>(size));
    nanocdr::Decoder cdr(buffer);

    for (const auto& field : msg_schema_->fields) {
      ARROW_RETURN_NOT_OK(
          decodeField(cdr, field, *msg_schema_, "", out));
    }
  } catch (const std::runtime_error& ex) {
    return arrow::Status::Invalid(
        std::string("CDR decode error: ") + ex.what());
  }

  return arrow::Status::OK();
}

}  // namespace mosaico
