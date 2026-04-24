/***** MIT License ****
 *
 *   Copyright (c) 2016-2024 Davide Faconti
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include "rosx_introspection/ros_parser.hpp"

#include <functional>
#include <limits>
#include <type_traits>

#include "rosx_introspection/flat_message_writer.hpp"

#ifdef ROSX_HAS_JSON
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rosx_introspection/deserializer.hpp"
#endif

namespace RosMsgParser {
inline bool operator==(const std::string& a, const std::string_view& b) {
  return (a.size() == b.size() && std::strncmp(a.data(), b.data(), a.size()) == 0);
}

static uint16_t fieldDepth(const FieldTreeNode* node) {
  uint16_t depth = 0;
  for (const auto* current = node; current != nullptr && current->parent() != nullptr; current = current->parent()) {
    depth++;
  }
  return depth;
}

static bool hasDirectKeyField(const ROSMessage* msg) {
  if (!msg) {
    return false;
  }
  for (const auto& field : msg->fields()) {
    if (!field.isConstant() && field.isKey()) {
      return true;
    }
  }
  return false;
}

static const std::string* enumNameForDDSCompatValue(const EnumDefinition* enum_def, int32_t enum_int) {
  if (!enum_def) {
    return nullptr;
  }
  for (const auto& ev : enum_def->values) {
    if (ev.ddsCompatValue() == enum_int) {
      return &ev.name;
    }
  }
  return nullptr;
}

static void appendKeySuffix(FieldLeaf& leaf, uint16_t depth, const char* text, int len) {
  if (len <= 0) {
    return;
  }
  KeySuffixAtDepth entry;
  entry.depth = depth;
  entry.suffix.assign(text, static_cast<size_t>(len));
  leaf.key_suffixes.push_back(entry);
}

Parser::Parser(
    const std::string& topic_name, const ROSType& msg_type, const std::string& definition, SchemaFormat format)
    : _global_warnings(&std::cerr),
      _topic_name(topic_name),
      _discard_large_array(DISCARD_LARGE_ARRAYS),
      _max_array_size(100),
      _blob_policy(STORE_BLOB_AS_COPY),
      _dummy_root_field(new ROSField(msg_type, topic_name)) {
  if (format == DDS_IDL) {
    _schema = ParseIDL(topic_name, msg_type, definition);
  } else {
    auto parsed_msgs = ParseMessageDefinitions(definition, msg_type);
    _schema = BuildMessageSchema(topic_name, parsed_msgs);
  }
}

const std::shared_ptr<MessageSchema>& Parser::getSchema() const {
  return _schema;
}

ROSMessage::Ptr Parser::getMessageByType(const ROSType& type) const {
  for (const auto& [msg_type, msg] : _schema->msg_library) {
    if (msg_type == type) {
      return msg;
    }
  }
  return {};
}

//=============================================================================
// Unified schema walk: walkSchema()
//=============================================================================

bool Parser::walkSchema(Span<const uint8_t> buffer, Deserializer* deserializer, MessageWriter* writer) const {
  deserializer->init(buffer);

  WalkState state;
  state.deserializer = deserializer;
  state.writer = writer;

  FieldLeaf rootnode;
  rootnode.node = _schema->field_tree.croot();
  auto root_msg = _schema->field_tree.croot()->value()->getMessagePtr(_schema->msg_library);

  walkImpl(root_msg.get(), rootnode, true, state);
  writer->finish();

  return state.entire_message_parsed;
}

// Opt A: Direct method instead of std::function (eliminates type erasure + indirect call)
// Opt B: FieldLeaf by reference with save/restore (eliminates SmallVector + string copy per field)
// Opt C: snprintf for key suffixes (eliminates std::to_string temporaries)
void Parser::walkImpl(const ROSMessage* msg, FieldLeaf& leaf, bool store, WalkState& state) const {
  auto* deserializer = state.deserializer;
  auto* writer = state.writer;
  size_t index_s = 0;
  const auto pre_key_suffix_size = leaf.key_suffixes.size();
  const auto current_depth = fieldDepth(leaf.node);

  // --- @key fields: process first, add path suffixes ---
  for (size_t fi = 0; fi < msg->fields().size(); fi++) {
    const ROSField& field = msg->field(fi);
    if (field.isConstant() || !field.isKey()) {
      continue;
    }

    char buf[96];
    if (field.type().typeID() == STRING) {
      std::string str;
      deserializer->deserializeString(str);
      int len = snprintf(buf, sizeof(buf), "[%s]", str.c_str());
      appendKeySuffix(leaf, current_depth, buf, len);
    } else if (field.getEnum() != nullptr) {
      Variant var = deserializer->deserialize(INT32);
      int32_t enum_int = var.convert<int32_t>();
      const std::string* enum_name = enumNameForDDSCompatValue(field.getEnum(), enum_int);
      if (enum_name) {
        int len = snprintf(buf, sizeof(buf), "[%s]", enum_name->c_str());
        appendKeySuffix(leaf, current_depth, buf, len);
      } else {
        int len = snprintf(buf, sizeof(buf), "[%d]", enum_int);
        appendKeySuffix(leaf, current_depth, buf, len);
      }
    } else if (field.type().isBuiltin()) {
      Variant var = deserializer->deserialize(field.type().typeID());
      int len = snprintf(buf, sizeof(buf), "[%s:%ld]", field.name().c_str(), (long)var.convert<int64_t>());
      appendKeySuffix(leaf, current_depth, buf, len);
    }
  }

  // Save leaf state for restore after each field (Opt B)
  const auto* saved_node = leaf.node;
  const auto saved_idx_size = leaf.index_array.size();
  const auto saved_idx_depth_size = leaf.index_depth_array.size();
  const auto saved_key_suffix_size = leaf.key_suffixes.size();

  // --- Process non-key fields ---
  for (const ROSField& field : msg->fields()) {
    bool DO_STORE = store;
    if (field.isConstant()) {
      continue;
    }
    if (field.isKey()) {
      index_s++;
      continue;
    }

    const ROSType& field_type = field.type();

    if (field.isOptional()) {
      if (!deserializer->hasOptionalMember()) {
        index_s++;
        continue;
      }
    }

    // Mutate leaf in-place instead of copying (Opt B)
    leaf.node = saved_node->child(index_s);
    const auto array_depth = fieldDepth(leaf.node);

    int32_t array_size = field.arraySize();
    if (array_size == -1) {
      array_size = deserializer->deserializeUInt32();
    }

    const bool is_array = field.isArray();
    const auto& dims = field.arrayDimensions();
    const bool is_multidim = dims.size() > 1;
    const auto field_msg = (!field_type.isBuiltin() && field.getEnum() == nullptr && field.getUnion() == nullptr)
                               ? field.getMessagePtr(_schema->msg_library)
                               : std::shared_ptr<ROSMessage>{};
    const bool suppress_array_index = is_array && hasDirectKeyField(field_msg.get());

    if (is_array && !suppress_array_index) {
      if (is_multidim) {
        // Push one index entry per dimension
        for (size_t d = 0; d < dims.size(); d++) {
          leaf.index_array.push_back(0);
          leaf.index_depth_array.push_back(array_depth);
        }
      } else {
        leaf.index_array.push_back(0);
        leaf.index_depth_array.push_back(array_depth);
      }
    }

    bool IS_BLOB = false;

    if (array_size > static_cast<int32_t>(_max_array_size)) {
      if (builtinSize(field_type.typeID()) == 1) {
        IS_BLOB = true;
      } else {
        if (_discard_large_array) {
          DO_STORE = false;
        }
        state.entire_message_parsed = false;
      }
    }

    if (IS_BLOB) {
      if (array_size > static_cast<int32_t>(deserializer->bytesLeft())) {
        throw std::runtime_error("Buffer overrun in walkSchema (blob)");
      }
      if (DO_STORE) {
        writer->writeBlob(leaf, Span<const uint8_t>(deserializer->getCurrentPtr(), array_size));
      }
      deserializer->jump(array_size);
    } else {
      bool DO_STORE_ARRAY = DO_STORE;
      for (int i = 0; i < array_size; i++) {
        if (DO_STORE_ARRAY && i >= static_cast<int32_t>(_max_array_size)) {
          DO_STORE_ARRAY = false;
        }
        if (is_array && !suppress_array_index && DO_STORE_ARRAY) {
          if (is_multidim) {
            // Compute multi-dimensional indices from flat index
            int flat = i;
            for (int d = static_cast<int>(dims.size()) - 1; d >= 0; d--) {
              leaf.index_array[saved_idx_size + d] = flat % dims[d];
              flat /= dims[d];
            }
          } else {
            leaf.index_array.back() = i;
          }
        }

        if (field.getEnum() != nullptr) {
          Variant var = deserializer->deserialize(INT32);
          if (DO_STORE_ARRAY) {
            int32_t enum_int = var.convert<int32_t>();
            const std::string* enum_name = enumNameForDDSCompatValue(field.getEnum(), enum_int);
            static const std::string empty_str;
            writer->writeEnum(leaf, enum_int, enum_name ? *enum_name : empty_str);
          }
        } else if (field.getUnion() != nullptr) {
          const auto* union_def = field.getUnion();
          auto disc_type = toBuiltinType(union_def->discriminant_type);
          std::string disc_value_str;

          if (disc_type == OTHER) {
            Variant disc_var = deserializer->deserialize(INT32);
            int32_t disc_int = disc_var.convert<int32_t>();
            auto enum_it = _schema->enum_library.find(ROSType(union_def->discriminant_type));
            if (enum_it != _schema->enum_library.end()) {
              for (const auto& ev : enum_it->second.values) {
                if (ev.value == disc_int) {
                  disc_value_str = ev.name;
                  break;
                }
              }
            }
            if (disc_value_str.empty()) {
              char buf[32];
              snprintf(buf, sizeof(buf), "%d", disc_int);
              disc_value_str = buf;
            }
          } else {
            Variant disc_var = deserializer->deserialize(disc_type);
            char buf[32];
            snprintf(buf, sizeof(buf), "%ld", (long)disc_var.convert<int64_t>());
            disc_value_str = buf;
          }

          auto case_it = union_def->cases.find(disc_value_str);
          const UnionCaseField* active_case = nullptr;
          if (case_it != union_def->cases.end()) {
            active_case = &case_it->second;
          } else if (union_def->default_case) {
            active_case = &union_def->default_case.value();
          }

          if (active_case) {
            if (active_case->type.typeID() == STRING) {
              std::string str;
              deserializer->deserializeString(str);
              if (DO_STORE_ARRAY) {
                writer->writeString(leaf, str);
              }
            } else if (active_case->type.isBuiltin()) {
              Variant var = deserializer->deserialize(active_case->type.typeID());
              if (DO_STORE_ARRAY) {
                writer->writeValue(leaf, var);
              }
            } else {
              auto case_msg_it = _schema->msg_library.find(active_case->type);
              if (case_msg_it != _schema->msg_library.end()) {
                writer->beginStruct(field);
                walkImpl(case_msg_it->second.get(), leaf, DO_STORE_ARRAY, state);
                writer->endStruct();
              }
            }
          }
        } else if (field_type.typeID() == STRING) {
          std::string str;
          deserializer->deserializeString(str);
          if (DO_STORE_ARRAY) {
            writer->writeString(leaf, str);
          }
        } else if (field_type.isBuiltin()) {
          Variant var = deserializer->deserialize(field_type.typeID());
          if (DO_STORE_ARRAY) {
            writer->writeValue(leaf, var);
          }
        } else {
          writer->beginStruct(field);
          walkImpl(field_msg.get(), leaf, DO_STORE_ARRAY, state);
          writer->endStruct();
        }
      }
    }

    // Restore leaf state (Opt B)
    leaf.index_array.resize(saved_idx_size);
    leaf.index_depth_array.resize(saved_idx_depth_size);
    leaf.key_suffixes.resize(saved_key_suffix_size);
    index_s++;
  }

  // Restore node pointer
  leaf.node = saved_node;
  leaf.key_suffixes.resize(pre_key_suffix_size);
}

// Opt D: Estimate field count for pre-reservation
static size_t estimateFieldCount(const ROSMessage* msg, const RosMessageLibrary& lib, int depth = 0) {
  if (depth > 10) {
    return 0;  // prevent infinite recursion
  }
  size_t count = 0;
  for (const auto& field : msg->fields()) {
    if (field.isConstant()) {
      continue;
    }
    if (field.type().isBuiltin() || field.getEnum() || field.getUnion()) {
      count++;
    } else {
      auto it = lib.find(field.type());
      if (it != lib.end()) {
        count += estimateFieldCount(it->second.get(), lib, depth + 1);
      }
    }
  }
  return count;
}

bool Parser::deserialize(Span<const uint8_t> buffer, FlatMessage* flat_container, Deserializer* deserializer) const {
  flat_container->schema = _schema;

  // Opt D: pre-reserve based on schema field count (cached after first call)
  if (_estimated_field_count == 0) {
    auto root_msg = _schema->field_tree.croot()->value()->getMessagePtr(_schema->msg_library);
    if (root_msg) {
      _estimated_field_count = estimateFieldCount(root_msg.get(), _schema->msg_library);
    }
  }
  if (flat_container->value.capacity() < _estimated_field_count) {
    flat_container->value.reserve(_estimated_field_count);
  }

  FlatMessageWriter writer(flat_container, _blob_policy);
  return walkSchema(buffer, deserializer, &writer);
}

//=============================================================================
// JSON support
//=============================================================================

#ifndef ROSX_HAS_JSON
bool Parser::deserializeIntoJson(
    Span<const uint8_t> buffer, std::string* json_txt, Deserializer* deserializer, int indent,
    bool ignore_constants) const {
  throw std::runtime_error("This version of rosx_introspection was built without JSON support");
  return false;
}
bool Parser::serializeFromJson(const std::string_view json_string, Serializer* serializer) const {
  throw std::runtime_error("This version of rosx_introspection was built without JSON support");
  return false;
}

#else

// For now, deserializeIntoJson uses the legacy implementation with IDL support added.
// A full JsonMessageWriter that builds hierarchical JSON from the walkSchema events
// is planned as a follow-up.
bool Parser::deserializeIntoJson(
    Span<const uint8_t> buffer, std::string* json_txt, Deserializer* deserializer, int indent,
    bool ignore_constants) const {
  deserializer->init(buffer);

  rapidjson::Document json_document;
  rapidjson::Document::AllocatorType& alloc = json_document.GetAllocator();

  std::function<void(const ROSMessage*, rapidjson::Value&)> deserializeImpl;

  deserializeImpl = [&](const ROSMessage* msg_node, rapidjson::Value& json_value) {
    for (const ROSField& field : msg_node->fields()) {
      if (field.isConstant() && ignore_constants) {
        continue;
      }
      // Note: @key fields are included in JSON output as regular fields.
      // In FlatMessage they become path suffixes, but JSON has no path concept.

      const ROSType& field_type = field.type();
      auto field_name = rapidjson::StringRef(field.name().data(), field.name().size());

      // Handle @optional fields
      if (field.isOptional()) {
        bool is_present = deserializer->hasOptionalMember();
        if (!is_present) {
          continue;
        }
      }

      int32_t array_size = field.arraySize();
      if (array_size == -1) {
        array_size = deserializer->deserializeUInt32();
      }

      const bool skip_large_byte_array = field.isArray() && array_size > static_cast<int32_t>(_max_array_size) &&
                                         field_type.isBuiltin() && builtinSize(field_type.typeID()) == 1;

      if (skip_large_byte_array) {
        if (array_size > static_cast<int32_t>(deserializer->bytesLeft())) {
          throw std::runtime_error("Buffer overrun in blob");
        }
        deserializer->jump(array_size);
      } else {
        rapidjson::Value array_value(rapidjson::kArrayType);

        for (int i = 0; i < array_size; i++) {
          rapidjson::Value new_value;
          new_value.SetObject();

          // Handle enum fields
          if (field.getEnum() != nullptr) {
            new_value.SetInt(deserializer->deserialize(INT32).convert<int32_t>());
          }
          // Handle union fields
          else if (field.getUnion() != nullptr) {
            const auto* union_def = field.getUnion();
            auto disc_type = toBuiltinType(union_def->discriminant_type);
            std::string disc_value_str;

            if (disc_type == OTHER) {
              Variant disc_var = deserializer->deserialize(INT32);
              int32_t disc_int = disc_var.convert<int32_t>();
              auto enum_it = _schema->enum_library.find(ROSType(union_def->discriminant_type));
              if (enum_it != _schema->enum_library.end()) {
                for (const auto& ev : enum_it->second.values) {
                  if (ev.value == disc_int) {
                    disc_value_str = ev.name;
                    break;
                  }
                }
              }
              if (disc_value_str.empty()) {
                disc_value_str = std::to_string(disc_int);
              }
            } else {
              Variant disc_var = deserializer->deserialize(disc_type);
              disc_value_str = std::to_string(disc_var.convert<int64_t>());
            }

            auto case_it = union_def->cases.find(disc_value_str);
            const UnionCaseField* active_case = nullptr;
            if (case_it != union_def->cases.end()) {
              active_case = &case_it->second;
            } else if (union_def->default_case) {
              active_case = &union_def->default_case.value();
            }

            if (active_case) {
              if (active_case->type.typeID() == STRING) {
                std::string s;
                deserializer->deserializeString(s);
                new_value.SetString(s.c_str(), s.length(), alloc);
              } else if (active_case->type.isBuiltin()) {
                auto v = deserializer->deserialize(active_case->type.typeID());
                switch (active_case->type.typeID()) {
                  case FLOAT32:
                    new_value.SetFloat(v.convert<float>());
                    break;
                  case FLOAT64:
                    new_value.SetDouble(v.convert<double>());
                    break;
                  case UINT64:
                    new_value.SetUint64(v.convert<uint64_t>());
                    break;
                  case INT64:
                    new_value.SetInt64(v.convert<int64_t>());
                    break;
                  default:
                    new_value.SetInt(v.convert<int32_t>());
                    break;
                }
              } else {
                auto case_msg_it = _schema->msg_library.find(active_case->type);
                if (case_msg_it != _schema->msg_library.end()) {
                  deserializeImpl(case_msg_it->second.get(), new_value);
                }
              }
            }
          } else {
            // Standard field handling
            switch (field_type.typeID()) {
              case BOOL:
                new_value.SetBool(deserializer->deserialize(field_type.typeID()).convert<uint8_t>());
                break;
              case CHAR: {
                char c = deserializer->deserialize(field_type.typeID()).convert<int8_t>();
                new_value.SetString(&c, 1, alloc);
              } break;
              case BYTE:
              case UINT8:
              case UINT16:
              case UINT32:
                new_value.SetUint(deserializer->deserialize(field_type.typeID()).convert<uint32_t>());
                break;
              case UINT64:
                new_value.SetUint64(deserializer->deserialize(field_type.typeID()).convert<uint64_t>());
                break;
              case INT8:
              case INT16:
              case INT32:
                new_value.SetInt(deserializer->deserialize(field_type.typeID()).convert<int32_t>());
                break;
              case INT64:
                new_value.SetInt64(deserializer->deserialize(field_type.typeID()).convert<int64_t>());
                break;
              case FLOAT32:
                new_value.SetFloat(deserializer->deserialize(field_type.typeID()).convert<float>());
                break;
              case FLOAT64:
                new_value.SetDouble(deserializer->deserialize(field_type.typeID()).convert<double>());
                break;
              case TIME:
              case DURATION: {
                int sec = deserializer->deserialize(INT32).convert<int32_t>();
                int nsec = deserializer->deserialize(INT32).convert<int32_t>();
                rapidjson::Value sec_val;
                sec_val.SetInt(sec);
                new_value.AddMember("secs", sec_val, alloc);
                rapidjson::Value nsec_val;
                nsec_val.SetInt(nsec);
                new_value.AddMember("nsecs", nsec_val, alloc);
              } break;
              case STRING: {
                std::string s;
                deserializer->deserializeString(s);
                new_value.SetString(s.c_str(), s.length(), alloc);
              } break;
              case OTHER: {
                auto msg_node_child = field.getMessagePtr(_schema->msg_library);
                deserializeImpl(msg_node_child.get(), new_value);
              } break;
            }
          }

          if (field.isArray()) {
            array_value.PushBack(new_value, alloc);
          } else {
            json_value.AddMember(field_name, new_value, alloc);
          }
        }

        if (field.isArray()) {
          json_value.AddMember(field_name, array_value, alloc);
        }
      }
    }
  };

  rapidjson::Value& json_node = json_document.SetObject();
  auto root_msg = _schema->field_tree.croot()->value()->getMessagePtr(_schema->msg_library);
  deserializeImpl(root_msg.get(), json_node);

  thread_local rapidjson::StringBuffer json_buffer;
  json_buffer.Reserve(65536);
  json_buffer.Clear();

  if (indent == 0) {
    rapidjson::Writer<
        rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator,
        rapidjson::kWriteDefaultFlags | rapidjson::kWriteNanAndInfFlag>
        json_writer(json_buffer);
    json_document.Accept(json_writer);
    *json_txt = json_buffer.GetString();
  } else {
    rapidjson::PrettyWriter<rapidjson::StringBuffer> json_writer(json_buffer);
    json_writer.SetIndent(' ', indent);
    json_document.Accept(json_writer);
    *json_txt = json_buffer.GetString();
  }

  return true;
}

//-----------------------------------------------------------------------------
// serializeFromJson
//-----------------------------------------------------------------------------

namespace {
template <typename T>
T readJsonValue(rapidjson::Value* value_field, std::string_view field_name) {
  if (!value_field) {
    return T{};
  }
  if constexpr (std::is_unsigned_v<T> && std::is_integral_v<T>) {
    if (!value_field->IsUint64()) {
      throw std::runtime_error("Expected unsigned integer in field: " + std::string(field_name));
    }
    auto v = value_field->GetUint64();
    if (v > std::numeric_limits<T>::max()) {
      throw std::runtime_error("Value out of range in field: " + std::string(field_name));
    }
    return static_cast<T>(v);
  } else if constexpr (std::is_signed_v<T> && std::is_integral_v<T>) {
    if (!value_field->IsInt64()) {
      throw std::runtime_error("Expected integer in field: " + std::string(field_name));
    }
    auto v = value_field->GetInt64();
    if (v < std::numeric_limits<T>::min() || v > std::numeric_limits<T>::max()) {
      throw std::runtime_error("Value out of range in field: " + std::string(field_name));
    }
    return static_cast<T>(v);
  } else {
    static_assert(std::is_floating_point_v<T>);
    if (!value_field->IsNumber()) {
      throw std::runtime_error("Expected number in field: " + std::string(field_name));
    }
    return static_cast<T>(value_field->GetDouble());
  }
}
}  // namespace

bool Parser::serializeFromJson(const std::string_view json_string, Serializer* serializer) const {
  rapidjson::Document json_document;
  json_document.Parse(json_string.data(), json_string.size());
  if (json_document.HasParseError()) {
    throw std::runtime_error("Failed to parse JSON input");
  }
  if (!json_document.IsObject()) {
    throw std::runtime_error("JSON root must be an object");
  }
  serializer->reset();

  std::function<void(const ROSMessage*, rapidjson::Value*)> serializeImpl;

  serializeImpl = [&](const ROSMessage* msg_node, rapidjson::Value* json_value) {
    if (json_value && !json_value->IsObject()) {
      throw std::runtime_error("Expected JSON object while serializing nested message");
    }

    for (const ROSField& field : msg_node->fields()) {
      if (field.isConstant()) {
        continue;
      }
      const ROSType& field_type = field.type();
      const auto field_name = rapidjson::StringRef(field.name().data(), field.name().size());

      // Handle enum fields: serialize as INT32
      if (field.getEnum() != nullptr) {
        rapidjson::Value* field_json_value = nullptr;
        if (json_value && json_value->HasMember(field_name.s)) {
          field_json_value = &((*json_value)[field_name.s]);
        }
        int32_t val = field_json_value ? readJsonValue<int32_t>(field_json_value, field.name()) : 0;
        serializer->serialize(INT32, Variant(val));
        continue;
      }

      // Handle union fields: not supported in JSON serialization
      if (field.getUnion() != nullptr) {
        throw std::runtime_error("serializeFromJson not supported for union fields: " + field.name());
      }

      rapidjson::Value* field_json_value = nullptr;
      if (json_value && json_value->HasMember(field_name.s)) {
        field_json_value = &((*json_value)[field_name.s]);
      }

      const bool has_json_value = (field_json_value != nullptr);
      const bool is_array = field.isArray();
      const bool is_dynamic_array = is_array && field.arraySize() == -1;
      const bool is_fixed_array = is_array && field.arraySize() != -1;

      if (has_json_value && (is_array != field_json_value->IsArray())) {
        throw std::runtime_error(std::string("IsArray() mismatch in field: ") + field.name());
      }

      uint32_t array_size = field.arraySize();
      if (is_dynamic_array) {
        array_size = has_json_value ? static_cast<uint32_t>(field_json_value->GetArray().Size()) : 0;
        serializer->serializeUInt32(array_size);
      }
      if (has_json_value && is_fixed_array) {
        auto actual_size = static_cast<uint32_t>(field_json_value->GetArray().Size());
        if (array_size != actual_size) {
          throw std::runtime_error(std::string("Fixed array size mismatch in field: ") + field.name());
        }
      }

      const auto type_id = field_type.typeID();
      const auto fname = field.name();

      for (uint32_t i = 0; i < array_size; i++) {
        rapidjson::Value* value_field = nullptr;
        if (has_json_value) {
          value_field = is_array ? &(field_json_value->GetArray()[i]) : field_json_value;
        }

        switch (type_id) {
          case BOOL: {
            bool value = value_field ? value_field->GetBool() : false;
            serializer->serialize(type_id, value);
          } break;
          case CHAR: {
            char c = '\0';
            if (value_field && value_field->IsString() && value_field->GetStringLength() > 0) {
              c = value_field->GetString()[0];
            }
            serializer->serialize(type_id, c);
          } break;
          case BYTE:
          case UINT8:
            serializer->serialize(type_id, readJsonValue<uint8_t>(value_field, fname));
            break;
          case UINT16:
            serializer->serialize(type_id, readJsonValue<uint16_t>(value_field, fname));
            break;
          case UINT32:
            serializer->serialize(type_id, readJsonValue<uint32_t>(value_field, fname));
            break;
          case UINT64:
            serializer->serialize(type_id, readJsonValue<uint64_t>(value_field, fname));
            break;
          case INT8:
            serializer->serialize(type_id, readJsonValue<int8_t>(value_field, fname));
            break;
          case INT16:
            serializer->serialize(type_id, readJsonValue<int16_t>(value_field, fname));
            break;
          case INT32:
            serializer->serialize(type_id, readJsonValue<int32_t>(value_field, fname));
            break;
          case INT64:
            serializer->serialize(type_id, readJsonValue<int64_t>(value_field, fname));
            break;
          case FLOAT32:
            serializer->serialize(type_id, readJsonValue<float>(value_field, fname));
            break;
          case FLOAT64:
            serializer->serialize(type_id, readJsonValue<double>(value_field, fname));
            break;
          case DURATION:
          case TIME: {
            int32_t secs = 0;
            int32_t nsecs = 0;
            if (value_field) {
              if (!value_field->IsObject()) {
                throw std::runtime_error(std::string("Expected time/duration object in field: ") + fname);
              }
              auto sec_it = value_field->FindMember("secs");
              auto nsec_it = value_field->FindMember("nsecs");
              if (sec_it == value_field->MemberEnd() || nsec_it == value_field->MemberEnd()) {
                throw std::runtime_error(std::string("Missing secs/nsecs in field: ") + fname);
              }
              secs = readJsonValue<int32_t>(&sec_it->value, fname);
              nsecs = readJsonValue<int32_t>(&nsec_it->value, fname);
            }
            serializer->serializeUInt32(static_cast<uint32_t>(secs));
            serializer->serializeUInt32(static_cast<uint32_t>(nsecs));
          } break;
          case STRING: {
            if (value_field) {
              const char* str = value_field->GetString();
              uint32_t len = value_field->GetStringLength();
              serializer->serializeString(std::string(str, len));
            } else {
              serializer->serializeString("");
            }
          } break;
          case OTHER: {
            auto msg_node_child = field.getMessagePtr(_schema->msg_library);
            if (!msg_node_child) {
              throw std::runtime_error(std::string("Missing ROSType in library for field: ") + fname);
            }
            serializeImpl(msg_node_child.get(), value_field);
          } break;
        }
      }
    }
  };

  auto root_msg = _schema->field_tree.croot()->value()->getMessagePtr(_schema->msg_library);
  rapidjson::Value& json_root = json_document;
  serializeImpl(root_msg.get(), &json_root);

  return true;
}

#endif

//=============================================================================
// applyVisitorToBuffer (unchanged from original)
//=============================================================================

void Parser::applyVisitorToBuffer(
    const ROSType& /*msg_type*/, Span<uint8_t>& /*buffer*/, VisitingCallback /*callback*/) const {
  throw std::runtime_error("applyVisitorToBuffer is not implemented");
}

}  // namespace RosMsgParser
