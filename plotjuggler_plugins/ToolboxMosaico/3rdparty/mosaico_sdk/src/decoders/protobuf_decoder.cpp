// tools/decoders/protobuf_decoder.cpp
#include "decoders/protobuf_decoder.hpp"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>

#include <string>

namespace gpb = google::protobuf;

namespace mosaico {
namespace {

// Maximum nesting depth for the descriptor-driven walk. Self-referential
// protobuf schemas (e.g. `message Node { Node child = 1; }`, or
// FileDescriptorProto itself) would otherwise recurse without bound on any
// payload — even an empty one — because GetMessage on an unset sub-message
// field returns the default instance whose descriptor still declares the
// same self-referential field. A stack overflow is uncatchable, so we bail
// with an empty result past this cap.
constexpr int kMaxProtobufDepth = 64;

// Forward declarations.
void decodeMessage(const gpb::Message& msg,
                   const gpb::Descriptor* desc,
                   const std::string& prefix,
                   FieldMap& out,
                   int depth);
void decodeField(const gpb::Message& msg,
                 const gpb::FieldDescriptor* field,
                 const std::string& prefix,
                 FieldMap& out,
                 int depth);
void decodeRepeatedField(const gpb::Message& msg,
                         const gpb::Reflection* refl,
                         const gpb::FieldDescriptor* field,
                         const std::string& key,
                         FieldMap& out,
                         int depth);

// ---------------------------------------------------------------------------
// Decode a scalar (non-repeated) field value into the FieldMap.
// ---------------------------------------------------------------------------
void decodeScalarValue(const gpb::Message& msg,
                       const gpb::Reflection* refl,
                       const gpb::FieldDescriptor* field,
                       const std::string& key,
                       FieldMap& out,
                       int depth) {
  switch (field->cpp_type()) {
    case gpb::FieldDescriptor::CPPTYPE_DOUBLE:
      out.fields[key] = refl->GetDouble(msg, field);
      return;
    case gpb::FieldDescriptor::CPPTYPE_FLOAT:
      out.fields[key] = refl->GetFloat(msg, field);
      return;
    case gpb::FieldDescriptor::CPPTYPE_INT32:
      out.fields[key] = refl->GetInt32(msg, field);
      return;
    case gpb::FieldDescriptor::CPPTYPE_INT64:
      out.fields[key] = refl->GetInt64(msg, field);
      return;
    case gpb::FieldDescriptor::CPPTYPE_UINT32:
      out.fields[key] = refl->GetUInt32(msg, field);
      return;
    case gpb::FieldDescriptor::CPPTYPE_UINT64:
      out.fields[key] = refl->GetUInt64(msg, field);
      return;
    case gpb::FieldDescriptor::CPPTYPE_BOOL:
      out.fields[key] = refl->GetBool(msg, field);
      return;
    case gpb::FieldDescriptor::CPPTYPE_STRING: {
      auto val = refl->GetString(msg, field);
      if (field->type() == gpb::FieldDescriptor::TYPE_BYTES) {
        // Store bytes as vector<uint8_t>
        std::vector<uint8_t> bytes(val.begin(), val.end());
        out.fields[key] = std::move(bytes);
      } else {
        out.fields[key] = val;
      }
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_ENUM:
      out.fields[key] = refl->GetEnumValue(msg, field);
      return;
    case gpb::FieldDescriptor::CPPTYPE_MESSAGE: {
      // Skip unset sub-messages entirely. Without this gate, recursive
      // schemas would loop forever via the default sub-message instance.
      if (!refl->HasField(msg, field)) return;
      const auto& sub_msg = refl->GetMessage(msg, field);
      decodeMessage(sub_msg, field->message_type(), key, out, depth + 1);
      return;
    }
  }
}

// ---------------------------------------------------------------------------
// Decode a repeated field into the FieldMap.
// Numeric repeated fields -> vector<double>
// String repeated fields -> vector<string>
// Message repeated fields -> vector<FieldMap>
// ---------------------------------------------------------------------------
void decodeRepeatedField(const gpb::Message& msg,
                         const gpb::Reflection* refl,
                         const gpb::FieldDescriptor* field,
                         const std::string& key,
                         FieldMap& out,
                         int depth) {
  int count = refl->FieldSize(msg, field);

  switch (field->cpp_type()) {
    case gpb::FieldDescriptor::CPPTYPE_DOUBLE: {
      std::vector<double> vals;
      vals.reserve(count);
      for (int i = 0; i < count; ++i)
        vals.push_back(refl->GetRepeatedDouble(msg, field, i));
      out.fields[key] = std::move(vals);
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_FLOAT: {
      std::vector<double> vals;
      vals.reserve(count);
      for (int i = 0; i < count; ++i)
        vals.push_back(static_cast<double>(refl->GetRepeatedFloat(msg, field, i)));
      out.fields[key] = std::move(vals);
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_INT32: {
      std::vector<double> vals;
      vals.reserve(count);
      for (int i = 0; i < count; ++i)
        vals.push_back(static_cast<double>(refl->GetRepeatedInt32(msg, field, i)));
      out.fields[key] = std::move(vals);
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_INT64: {
      std::vector<double> vals;
      vals.reserve(count);
      for (int i = 0; i < count; ++i)
        vals.push_back(static_cast<double>(refl->GetRepeatedInt64(msg, field, i)));
      out.fields[key] = std::move(vals);
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_UINT32: {
      std::vector<double> vals;
      vals.reserve(count);
      for (int i = 0; i < count; ++i)
        vals.push_back(static_cast<double>(refl->GetRepeatedUInt32(msg, field, i)));
      out.fields[key] = std::move(vals);
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_UINT64: {
      std::vector<double> vals;
      vals.reserve(count);
      for (int i = 0; i < count; ++i)
        vals.push_back(static_cast<double>(refl->GetRepeatedUInt64(msg, field, i)));
      out.fields[key] = std::move(vals);
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_BOOL: {
      std::vector<double> vals;
      vals.reserve(count);
      for (int i = 0; i < count; ++i)
        vals.push_back(refl->GetRepeatedBool(msg, field, i) ? 1.0 : 0.0);
      out.fields[key] = std::move(vals);
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_STRING: {
      if (field->type() == gpb::FieldDescriptor::TYPE_BYTES) {
        // repeated bytes -> vector<string> (each as string)
        std::vector<std::string> vals;
        vals.reserve(count);
        for (int i = 0; i < count; ++i)
          vals.push_back(refl->GetRepeatedString(msg, field, i));
        out.fields[key] = std::move(vals);
      } else {
        std::vector<std::string> vals;
        vals.reserve(count);
        for (int i = 0; i < count; ++i)
          vals.push_back(refl->GetRepeatedString(msg, field, i));
        out.fields[key] = std::move(vals);
      }
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_ENUM: {
      std::vector<double> vals;
      vals.reserve(count);
      for (int i = 0; i < count; ++i)
        vals.push_back(static_cast<double>(refl->GetRepeatedEnumValue(msg, field, i)));
      out.fields[key] = std::move(vals);
      return;
    }
    case gpb::FieldDescriptor::CPPTYPE_MESSAGE: {
      std::vector<FieldMap> elements;
      elements.reserve(count);
      for (int i = 0; i < count; ++i) {
        const auto& sub_msg = refl->GetRepeatedMessage(msg, field, i);
        FieldMap elem;
        decodeMessage(sub_msg, field->message_type(), "", elem, depth + 1);
        elements.push_back(std::move(elem));
      }
      out.fields[key] = std::move(elements);
      return;
    }
  }
}

// ---------------------------------------------------------------------------
// Decode a single field (scalar, repeated, or map).
// ---------------------------------------------------------------------------
void decodeField(const gpb::Message& msg,
                 const gpb::FieldDescriptor* field,
                 const std::string& prefix,
                 FieldMap& out,
                 int depth) {
  const auto* refl = msg.GetReflection();
  std::string key = prefix.empty() ? field->name()
                                   : prefix + "." + field->name();

  if (field->is_map()) {
    // map<K,V> -> vector<FieldMap> where each has "key" and "value"
    const auto* map_desc = field->message_type();
    const auto* key_field = map_desc->FindFieldByName("key");
    const auto* val_field = map_desc->FindFieldByName("value");

    int count = refl->FieldSize(msg, field);
    std::vector<FieldMap> entries;
    entries.reserve(count);
    for (int i = 0; i < count; ++i) {
      const auto& entry = refl->GetRepeatedMessage(msg, field, i);
      FieldMap entry_fm;
      const auto* entry_refl = entry.GetReflection();
      decodeScalarValue(entry, entry_refl, key_field, "key", entry_fm,
                        depth + 1);
      decodeScalarValue(entry, entry_refl, val_field, "value", entry_fm,
                        depth + 1);
      entries.push_back(std::move(entry_fm));
    }
    out.fields[key] = std::move(entries);
    return;
  }

  if (field->is_repeated()) {
    decodeRepeatedField(msg, refl, field, key, out, depth);
    return;
  }

  // Scalar field.
  decodeScalarValue(msg, refl, field, key, out, depth);
}

void decodeMessage(const gpb::Message& msg,
                   const gpb::Descriptor* desc,
                   const std::string& prefix,
                   FieldMap& out,
                   int depth) {
  if (depth >= kMaxProtobufDepth) return;
  for (int i = 0; i < desc->field_count(); ++i) {
    decodeField(msg, desc->field(i), prefix, out, depth);
  }
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

ProtobufDecoder::ProtobufDecoder() = default;
ProtobufDecoder::~ProtobufDecoder() = default;

std::vector<std::string> ProtobufDecoder::supportedEncodings() const {
  return {"protobuf"};
}

arrow::Status ProtobufDecoder::prepare(const DecoderContext& ctx) {
  gpb::FileDescriptorSet fds;
  if (!fds.ParseFromArray(ctx.schema_data.data(),
                          static_cast<int>(ctx.schema_data.size()))) {
    return arrow::Status::Invalid(
        "Failed to parse FileDescriptorSet from schema data");
  }

  pool_ = std::make_unique<gpb::DescriptorPool>();
  for (const auto& file : fds.file()) {
    if (pool_->BuildFile(file) == nullptr) {
      return arrow::Status::Invalid("Failed to build file descriptor: " +
                                    file.name());
    }
  }

  message_desc_ = pool_->FindMessageTypeByName(ctx.schema_name);
  if (message_desc_ == nullptr) {
    return arrow::Status::Invalid("Unknown protobuf message type: " +
                                  ctx.schema_name);
  }

  factory_ = std::make_unique<gpb::DynamicMessageFactory>(pool_.get());
  return arrow::Status::OK();
}

arrow::Status ProtobufDecoder::decode(
    const std::byte* data, uint64_t size,
    const DecoderContext& /*ctx*/,
    FieldMap& out) {
  if (message_desc_ == nullptr || factory_ == nullptr) {
    return arrow::Status::Invalid("prepare() must be called first");
  }

  const auto* prototype = factory_->GetPrototype(message_desc_);
  std::unique_ptr<gpb::Message> msg(prototype->New());
  if (!msg->ParseFromArray(data, static_cast<int>(size))) {
    return arrow::Status::Invalid("Failed to parse protobuf message");
  }

  decodeMessage(*msg, message_desc_, "", out, /*depth=*/0);
  return arrow::Status::OK();
}

}  // namespace mosaico
