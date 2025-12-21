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

#include <functional>

#include "rosx_introspection/ros_parser.hpp"
#include "rosx_introspection/deserializer.hpp"

#ifdef ROSX_JSON_PARSER

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/reader.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/memorystream.h"

#endif
namespace RosMsgParser
{
inline bool operator==(const std::string& a, const std::string_view& b)
{
  return (a.size() == b.size() && std::strncmp(a.data(), b.data(), a.size()) == 0);
}

Parser::Parser(const std::string& topic_name, const ROSType& msg_type,
               const std::string& definition)
  : _global_warnings(&std::cerr)
  , _topic_name(topic_name)
  , _discard_large_array(DISCARD_LARGE_ARRAYS)
  , _max_array_size(100)
  , _blob_policy(STORE_BLOB_AS_COPY)
  , _dummy_root_field(new ROSField(msg_type, topic_name))
{
  auto parsed_msgs = ParseMessageDefinitions(definition, msg_type);
  _schema = BuildMessageSchema(topic_name, parsed_msgs);
}

const std::shared_ptr<MessageSchema>& Parser::getSchema() const
{
  return _schema;
}

ROSMessage::Ptr Parser::getMessageByType(const ROSType& type) const
{
  for (const auto& [msg_type, msg] : _schema->msg_library)  // find in the list
  {
    if (msg_type == type)
    {
      return msg;
    }
  }
  return {};
}

template <typename Container>
inline void ExpandVectorIfNecessary(Container& container, size_t new_size)
{
  if (container.size() <= new_size)
  {
    const size_t increased_size = std::max(size_t(32), container.size() * 2);
    container.resize(increased_size);
  }
}

bool Parser::deserialize(Span<const uint8_t> buffer, FlatMessage* flat_container,
                         Deserializer* deserializer) const
{
  deserializer->init(buffer);

  bool entire_message_parse = true;

  size_t value_index = 0;
  size_t name_index = 0;
  size_t blob_index = 0;
  size_t blob_storage_index = 0;

  std::function<void(const ROSMessage*, FieldLeaf, bool)> deserializeImpl;

  deserializeImpl = [&](const ROSMessage* msg, FieldLeaf tree_leaf, bool store) {
    size_t index_s = 0;
    size_t index_m = 0;

    for (const ROSField& field : msg->fields())
    {
      bool DO_STORE = store;
      if (field.isConstant())
      {
        continue;
      }

      const ROSType& field_type = field.type();

      auto new_tree_leaf = tree_leaf;
      new_tree_leaf.node = tree_leaf.node->child(index_s);

      int32_t array_size = field.arraySize();
      if (array_size == -1)
      {
        array_size = deserializer->deserializeUInt32();
      }
      if (field.isArray())
      {
        new_tree_leaf.index_array.push_back(0);
      }

      bool IS_BLOB = false;

      // Stop storing it if is NOT a blob and a very large array.
      if (array_size > static_cast<int32_t>(_max_array_size) &&
          field_type.typeID() == BuiltinType::OTHER)
      {
        if (builtinSize(field_type.typeID()) == 1)
        {
          IS_BLOB = true;
        }
        else
        {
          if (_discard_large_array)
          {
            DO_STORE = false;
          }
          entire_message_parse = false;
        }
      }
      // special case. This is a "blob", typically an image, a map, pointcloud, etc.
      if (IS_BLOB)
      {
        ExpandVectorIfNecessary(flat_container->blob, blob_index);

        if (array_size > deserializer->bytesLeft())
        {
          throw std::runtime_error("Buffer overrun in deserializeIntoFlatContainer "
                                   "(blob)");
        }
        if (DO_STORE)
        {
          flat_container->blob[blob_index].first = FieldsVector(new_tree_leaf);
          auto& blob = flat_container->blob[blob_index].second;
          blob_index++;

          if (_blob_policy == STORE_BLOB_AS_COPY)
          {
            ExpandVectorIfNecessary(flat_container->blob_storage, blob_storage_index);

            auto& storage = flat_container->blob_storage[blob_storage_index];
            storage.resize(array_size);
            std::memcpy(storage.data(), deserializer->getCurrentPtr(), array_size);
            blob_storage_index++;

            blob = Span<const uint8_t>(storage.data(), storage.size());
          }
          else
          {
            blob = Span<const uint8_t>(deserializer->getCurrentPtr(), array_size);
          }
        }
        deserializer->jump(array_size);
      }
      else  // NOT a BLOB
      {
        bool DO_STORE_ARRAY = DO_STORE;
        for (int i = 0; i < array_size; i++)
        {
          if (DO_STORE_ARRAY && i >= static_cast<int32_t>(_max_array_size))
          {
            DO_STORE_ARRAY = false;
          }

          if (field.isArray() && DO_STORE_ARRAY)
          {
            new_tree_leaf.index_array.back() = i;
          }

          if (field_type.typeID() == STRING)
          {
            ExpandVectorIfNecessary(flat_container->name, name_index);

            std::string str;
            deserializer->deserializeString(str);

            if (DO_STORE_ARRAY)
            {
              flat_container->name[name_index].first = FieldsVector(new_tree_leaf);
              flat_container->name[name_index].second = str;
              name_index++;
            }
          }
          else if (field_type.isBuiltin())
          {
            ExpandVectorIfNecessary(flat_container->value, value_index);

            Variant var = deserializer->deserialize(field_type.typeID());
            if (DO_STORE_ARRAY)
            {
              flat_container->value[value_index] =
                  std::make_pair(new_tree_leaf, std::move(var));
              value_index++;
            }
          }
          else
          {  // field_type.typeID() == OTHER
            auto msg_node = field.getMessagePtr(_schema->msg_library);
            deserializeImpl(msg_node.get(), new_tree_leaf, DO_STORE_ARRAY);
          }
        }  // end for array_size
      }

      if (field_type.typeID() == OTHER)
      {
        index_m++;
      }
      index_s++;
    }  // end for fields
  };   // end of lambda

  // pass the shared_ptr
  flat_container->schema = _schema;

  FieldLeaf rootnode;
  rootnode.node = _schema->field_tree.croot();
  auto root_msg =
      _schema->field_tree.croot()->value()->getMessagePtr(_schema->msg_library);

  deserializeImpl(root_msg.get(), rootnode, true);

  flat_container->name.resize(name_index);
  flat_container->value.resize(value_index);
  flat_container->blob.resize(blob_index);
  flat_container->blob_storage.resize(blob_storage_index);

  return entire_message_parse;
}

#ifdef ROSX_JSON_PARSER

bool Parser::deserializeIntoJson(Span<const uint8_t> buffer, std::string* json_txt,
                                 Deserializer* deserializer, int indent,
                                 bool ignore_constants) const
{
  deserializer->init(buffer);

  rapidjson::Document json_document;
  rapidjson::Document::AllocatorType& alloc = json_document.GetAllocator();

  size_t buffer_offset = 0;

  std::function<void(const ROSMessage*, rapidjson::Value&)> deserializeImpl;

  deserializeImpl = [&](const ROSMessage* msg_node, rapidjson::Value& json_value) {
    size_t index_s = 0;
    size_t index_m = 0;

    for (const ROSField& field : msg_node->fields())
    {
      if (field.isConstant() && ignore_constants)
      {
        continue;
      }

      const ROSType& field_type = field.type();
      auto field_name = rapidjson::StringRef(field.name().data(), field.name().size());

      int32_t array_size = field.arraySize();
      if (array_size == -1)
      {
        array_size = deserializer->deserializeUInt32();
      }

      // Stop storing if it is a blob.
      if (array_size > static_cast<int32_t>(_max_array_size))
      {
        if (buffer_offset + array_size > static_cast<std::size_t>(buffer.size()))
        {
          throw std::runtime_error("Buffer overrun in blob");
        }
        buffer_offset += array_size;
      }
      else  // NOT a BLOB
      {
        rapidjson::Value array_value(rapidjson::kArrayType);

        for (int i = 0; i < array_size; i++)
        {
          rapidjson::Value new_value;
          new_value.SetObject();

          switch (field_type.typeID())
          {
            case BOOL:
              new_value.SetBool(
                  deserializer->deserialize(field_type.typeID()).convert<uint8_t>());
              break;
            case CHAR: {
              char c = deserializer->deserialize(field_type.typeID()).convert<int8_t>();
              new_value.SetString(&c, 1, alloc);
            }
            break;
            case BYTE:
            case UINT8:
            case UINT16:
            case UINT32:
              new_value.SetUint(
                  (deserializer->deserialize(field_type.typeID()).convert<uint32_t>()));
              break;
            case UINT64:
              new_value.SetUint64(
                  (deserializer->deserialize(field_type.typeID()).convert<uint64_t>()));
              break;
            case INT8:
            case INT16:
            case INT32:
              new_value.SetInt(
                  (deserializer->deserialize(field_type.typeID()).convert<int32_t>()));
              break;
            case INT64:
              new_value.SetInt64(
                  (deserializer->deserialize(field_type.typeID()).convert<int64_t>()));
              break;
            case FLOAT32:
              new_value.SetFloat(
                  (deserializer->deserialize(field_type.typeID()).convert<float>()));
              break;
            case FLOAT64:
              new_value.SetDouble(
                  (deserializer->deserialize(field_type.typeID()).convert<double>()));
              break;
            case TIME: {
              int sec = deserializer->deserialize(INT32).convert<int32_t>();
              int nsec = deserializer->deserialize(INT32).convert<int32_t>();
              rapidjson::Value sec_Value;
              sec_Value.SetObject();
              sec_Value.SetInt(sec);
              new_value.AddMember("secs", sec_Value, alloc);

              rapidjson::Value nsec_value;
              nsec_value.SetObject();
              nsec_value.SetInt(nsec);
              new_value.AddMember("nsecs", nsec_value, alloc);
            }
            break;
            case DURATION: {
              int sec = deserializer->deserialize(INT32).convert<int32_t>();
              int nsec = deserializer->deserialize(INT32).convert<int32_t>();
              rapidjson::Value sec_Value;
              sec_Value.SetObject();
              sec_Value.SetInt(sec);
              new_value.AddMember("secs", sec_Value, alloc);

              rapidjson::Value nsec_value;
              nsec_value.SetObject();
              nsec_value.SetInt(nsec);
              new_value.AddMember("nsecs", nsec_value, alloc);
            }
            break;

            case STRING: {
              std::string s;
              deserializer->deserializeString(s);
              new_value.SetString(s.c_str(), s.length(), alloc);
            }
            break;
            case OTHER: {
              auto msg_node_child = field.getMessagePtr(_schema->msg_library);
              deserializeImpl(msg_node_child.get(), new_value);
            }
            break;
          }  // end switch

          if (field.isArray())
          {
            array_value.PushBack(new_value, alloc);
          }
          else
          {
            json_value.AddMember(field_name, new_value, alloc);
          }
        }  // end for array

        if (field.isArray())
        {
          json_value.AddMember(field_name, array_value, alloc);
        }
      }  // end for array_size

      if (field_type.typeID() == OTHER)
      {
        index_m++;
      }
      index_s++;
    }  // end for fields
  };   // end of lambda

  FieldLeaf rootnode;
  rootnode.node = _schema->field_tree.croot();
  auto root_msg =
      _schema->field_tree.croot()->value()->getMessagePtr(_schema->msg_library);

  rapidjson::Value& json_node = json_document.SetObject();
  deserializeImpl(root_msg.get(), json_node);

  static rapidjson::StringBuffer json_buffer;
  json_buffer.Reserve(65536);
  json_buffer.Clear();

  if (indent == 0)
  {
    rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>,
                      rapidjson::CrtAllocator,
                      rapidjson::kWriteDefaultFlags | rapidjson::kWriteNanAndInfFlag>
        json_writer(json_buffer);
    json_document.Accept(json_writer);

    *json_txt = json_buffer.GetString();
  }
  else
  {
    rapidjson::PrettyWriter<rapidjson::StringBuffer> json_writer(json_buffer);
    json_writer.SetIndent(' ', indent);
    json_document.Accept(json_writer);
    *json_txt = json_buffer.GetString();
  }

  return true;
}

bool Parser::serializeFromJson(const std::string_view json_string,
                               Serializer* serializer) const
{
  rapidjson::Document json_document;
  json_document.Parse(json_string.data(), json_string.size());
  serializer->reset();

  std::function<void(const ROSMessage*, rapidjson::Value*)> serializeImpl;

  serializeImpl = [&](const ROSMessage* msg_node, rapidjson::Value* json_value) {
    size_t index_s = 0;
    size_t index_m = 0;

    for (const ROSField& field : msg_node->fields())
    {
      if (field.isConstant())
      {
        continue;
      }
      const ROSType& field_type = field.type();
      const auto field_name =
          rapidjson::StringRef(field.name().data(), field.name().size());

      const bool has_json_value = json_value && json_value->HasMember(field_name.s);
      const bool is_array = field.isArray();
      const bool is_dynamic_array = is_array && field.arraySize() == -1;
      const bool is_fixed_array = is_array && field.arraySize() != -1;

      // both must be array or not array
      if (has_json_value && (is_array != (*json_value)[field_name.s].IsArray()))
      {
        throw std::runtime_error(std::string("IsArray() mismatch in field: ") +
                                 field.name());
      }

      uint32_t array_size = field.arraySize();
      if (is_dynamic_array)
      {
        // if not present in the JSON, we will add an array size of 0
        array_size = has_json_value ? (*json_value)[field_name.s].GetArray().Size() : 0;
        serializer->serializeUInt32(array_size);
      }
      if (has_json_value && is_fixed_array)
      {
        int actual_size = (*json_value)[field_name.s].GetArray().Size();
        if (array_size != actual_size)
        {
          throw std::runtime_error(std::string("Fixed array size mismatch in field: ") +
                                   field.name());
        }
      }

      const auto type_id = field_type.typeID();

      for (int i = 0; i < array_size; i++)
      {
        // is !has_json_value , we will serialize a zero value
        rapidjson::Value zero_value = rapidjson::Value(0);
        rapidjson::Value* value_field = &zero_value;

        if (has_json_value)
        {
          value_field = is_array ? &((*json_value)[field_name.s].GetArray()[i]) :
                                   &((*json_value)[field_name.s]);
        }

        switch (type_id)
        {
          case BOOL:
            serializer->serialize(type_id, value_field->GetBool());
            break;
          case CHAR:
            serializer->serialize(type_id, value_field->GetString()[0]);
            break;

          case BYTE:
          case UINT8:
            serializer->serialize(type_id, uint8_t(value_field->GetUint()));
            break;
          case UINT16:
            serializer->serialize(type_id, uint16_t(value_field->GetUint()));
            break;
          case UINT32:
            serializer->serialize(type_id, value_field->GetUint());
            break;
          case UINT64:
            serializer->serialize(type_id, value_field->GetUint64());
            break;

            break;
          case INT8:
            serializer->serialize(type_id, int8_t(value_field->GetUint()));
            break;
          case INT16:
            serializer->serialize(type_id, int16_t(value_field->GetUint()));
            break;
          case INT32:
            serializer->serialize(type_id, value_field->GetInt());
            break;
          case INT64:
            serializer->serialize(type_id, value_field->GetInt64());
            break;

          case FLOAT32:
            serializer->serialize(type_id, value_field->GetFloat());
            break;
          case FLOAT64:
            serializer->serialize(type_id, value_field->GetDouble());
            break;

          case DURATION:
          case TIME: {
            uint32_t secs = value_field->GetObject()["secs"].GetInt();
            serializer->serializeUInt32(secs);

            uint32_t nsecs = value_field->GetObject()["nsecs"].GetInt();
            serializer->serializeUInt32(secs);
          }
          break;

          case STRING: {
            if (has_json_value)
            {
              const char* str = value_field->GetString();
              uint32_t len = value_field->GetStringLength();
              serializer->serializeString(std::string(str, len));
            }
            else
            {
              serializer->serializeString("");
            }
          }
          break;
          case OTHER: {
            auto msg_node_child = field.getMessagePtr(_schema->msg_library);
            if (!has_json_value)
            {
              serializeImpl(msg_node_child.get(), nullptr);
            }
            else
            {
              serializeImpl(msg_node_child.get(), value_field);
            }
          }
          break;
        }  // end switch
      }    // end for array

      if (field_type.typeID() == OTHER)
      {
        index_m++;
      }
      index_s++;
    }  // end for fields
  };   // end of lambda

  auto root_msg =
      _schema->field_tree.croot()->value()->getMessagePtr(_schema->msg_library);

  rapidjson::Value& json_root = json_document;
  serializeImpl(root_msg.get(), &json_root);

  return true;
}
#endif

}  // namespace RosMsgParser
