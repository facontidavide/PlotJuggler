#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace DataTamerParser
{

constexpr int SCHEMA_VERSION = 4;

enum class BasicType: uint8_t
{
  BOOL,
  CHAR,
  INT8,
  UINT8,

  INT16,
  UINT16,

  INT32,
  UINT32,

  INT64,
  UINT64,

  FLOAT32,
  FLOAT64,
  OTHER
};

constexpr size_t TypesCount = 13;

using VarNumber = std::variant<bool, char, int8_t, uint8_t, int16_t, uint16_t, int32_t,
                               uint32_t, int64_t, uint64_t, float, double>;

struct BufferSpan
{
  const uint8_t* data = nullptr;
  size_t size = 0;

  void trimFront(size_t n)
  {
    data += n;
    size -= n;
  }
};

VarNumber DeserializeToVarNumber(BasicType type, BufferSpan& buffer);

//---------------------------------------------------------
struct TypeField
{
  std::string field_name;
  BasicType type = BasicType::OTHER;
  std::string type_name;
  bool is_vector = 0;
  uint32_t array_size = 0;

  bool operator==(const TypeField& other) const;
  bool operator!=(const TypeField& other) const;
};

using FieldsVector = std::vector<TypeField>;

/**
 * @brief DataTamer uses a simple "flat" schema of key/value pairs (each pair is a "field").
 */
struct Schema
{
  uint64_t hash = 0;
  FieldsVector fields;
  std::string channel_name;

  std::map<std::string, FieldsVector> custom_types;
};

struct SnapshotView
{
  /// Unique identifier of the schema
  size_t schema_hash;

  /// snapshot timestamp
  uint64_t timestamp;

  /// Vector that tell us if a field of the schema is
  /// active or not. It is basically an optimized vector
  /// of bools, where each byte contains 8 boolean flags.
  BufferSpan active_mask;

  /// serialized data containing all the values, ordered as in the schema
  BufferSpan payload;
};

bool GetBit(BufferSpan mask, size_t index);

constexpr auto NullCustomCallback = [](const std::string&, const BufferSpan,
                                       const std::string&) {};

// Callback must be a std::function or lambda with signature:
//
// void(const std::string& name_field, const VarNumber& value)
//
// void(const std::string& name_field, const BufferSpan payload, const std::string& type_name)
//
template <typename NumberCallback, typename CustomCallback = decltype(NullCustomCallback)>
bool ParseSnapshot(const Schema& schema, SnapshotView snapshot,
                   const NumberCallback& callback_number,
                   const CustomCallback& callback_custom = NullCustomCallback);

//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

template <typename T>
inline T Deserialize(BufferSpan& buffer)
{
  T var;
  const auto N = sizeof(T);
  std::memcpy(&var, buffer.data, N);
  buffer.data += N;
  if(N > buffer.size)
  {
    throw std::runtime_error("Buffer overflow");
  }
  buffer.size -= N;
  return var;
}

inline VarNumber DeserializeToVarNumber(BasicType type, BufferSpan& buffer)
{
  switch(type)
  {
    case BasicType::BOOL:
      return Deserialize<bool>(buffer);
    case BasicType::CHAR:
      return Deserialize<char>(buffer);

    case BasicType::INT8:
      return Deserialize<int8_t>(buffer);
    case BasicType::UINT8:
      return Deserialize<uint8_t>(buffer);

    case BasicType::INT16:
      return Deserialize<int16_t>(buffer);
    case BasicType::UINT16:
      return Deserialize<uint16_t>(buffer);

    case BasicType::INT32:
      return Deserialize<int32_t>(buffer);
    case BasicType::UINT32:
      return Deserialize<uint32_t>(buffer);

    case BasicType::INT64:
      return Deserialize<int64_t>(buffer);
    case BasicType::UINT64:
      return Deserialize<uint64_t>(buffer);

    case BasicType::FLOAT32:
      return Deserialize<float>(buffer);
    case BasicType::FLOAT64:
      return Deserialize<double>(buffer);

    case BasicType::OTHER:
      return double(std::numeric_limits<double>::quiet_NaN());
  }
  return {};
}

inline bool GetBit(BufferSpan mask, size_t index)
{
  const uint8_t& byte = mask.data[index >> 3];
  return 0 != (byte & uint8_t(1 << (index % 8)));
}

[[nodiscard]] inline uint64_t AddFieldToHash(const TypeField& field, uint64_t hash)
{
  // https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
  const std::hash<std::string> str_hasher;
  const std::hash<uint8_t> type_hasher;
  const std::hash<bool> bool_hasher;
  const std::hash<uint32_t> uint_hasher;

  auto combine = [&hash](const auto& hasher, const auto& val) {
    hash ^= hasher(val) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  };

  combine(str_hasher, field.field_name);
  combine(type_hasher, static_cast<uint8_t>(field.type));
  if(field.type == BasicType::OTHER)
  {
    combine(str_hasher, field.type_name);
  }
  combine(bool_hasher, field.is_vector);
  combine(uint_hasher, field.array_size);
  return hash;
}

bool TypeField::operator==(const TypeField& other) const
{
  return is_vector == other.is_vector && type == other.type &&
         array_size == other.array_size && field_name == other.field_name &&
         type_name == other.type_name;
}

inline Schema BuilSchemaFromText(const std::string& txt, bool check_hash = false)
{
  auto trimString = [](std::string& str) {
    while(!str.empty() && (str.back() == ' ' || str.back() == '\r'))
    {
      str.pop_back();
    }
    while(!str.empty() && (str.front() == ' ' || str.front() == '\r'))
    {
      str.erase(0, 1);
    }
  };

  std::istringstream ss(txt);
  std::string line;
  Schema schema;
  uint64_t declared_schema = 0;

  std::vector<TypeField>* field_vector = &schema.fields;

  while(std::getline(ss, line))
  {
    trimString(line);
    if(line.empty())
    {
      continue;
    }
    if(line.find("==============================") != std::string::npos)
    {
      // get "MSG:" in the next line
      std::getline(ss, line);
      auto msg_pos = line.find("MSG: ");
      if(msg_pos == std::string::npos)
      {
        throw std::runtime_error("Expecting \"MSG: \" at the beginning of line: " + line);
      }
      line.erase(0, 5);
      trimString(line);
      field_vector = &schema.custom_types[line];
      continue;
    }

    // a single space is expected
    auto space_pos = line.find(' ');
    if(space_pos == std::string::npos)
    {
      throw std::runtime_error("Unexpected line: " + line);
    }
    if(line.find("### ") == 0)
    {
      space_pos = line.find(' ', 5);
    }

    std::string str_left = line.substr(0, space_pos);
    std::string str_right = line.substr(space_pos + 1, line.size() - (space_pos + 1));
    trimString(str_left);
    trimString(str_right);

    const std::string* str_type = &str_left;
    const std::string* str_name = &str_right;

    if(str_left == "### version:")
    {
      // check compatibility
      if(std::stoi(str_right) != SCHEMA_VERSION)
      {
        throw std::runtime_error("Wrong SCHEMA_VERSION");
      }
      continue;
    }
    if(str_left == "### hash:")
    {
      // check compatibility
      declared_schema = std::stoul(str_right);
      continue;
    }

    if(str_left == "### channel_name:")
    {
      // check compatibility
      schema.channel_name = str_right;
      schema.hash = std::hash<std::string>()(schema.channel_name);
      continue;
    }

    TypeField field;

    static const std::array<std::string, TypesCount> kNamesNew = {
      "bool",   "char",  "int8",   "uint8",   "int16",   "uint16", "int32",
      "uint32", "int64", "uint64", "float32", "float64", "other"
    };
    // backcompatibility to old format
    static const std::array<std::string, TypesCount> kNamesOld = {
      "BOOL",   "CHAR",  "INT8",   "UINT8", "INT16",  "UINT16", "INT32",
      "UINT32", "INT64", "UINT64", "FLOAT", "DOUBLE", "OTHER"
    };

    for(size_t i = 0; i < TypesCount; i++)
    {
      if(str_left.find(kNamesNew[i]) == 0)
      {
        field.type = static_cast<BasicType>(i);
        break;
      }
      if(str_right.find(kNamesOld[i]) == 0)
      {
        field.type = static_cast<BasicType>(i);
        std::swap(str_type, str_name);
        break;
      }
    }

    auto offset = str_type->find_first_of(" [");
    if(field.type != BasicType::OTHER)
    {
      field.type_name = kNamesNew[static_cast<size_t>(field.type)];
    }
    else
    {
      field.type_name = str_type->substr(0, offset);
    }

    if(offset != std::string::npos && str_type->at(offset) == '[')
    {
      field.is_vector = true;
      auto pos = str_type->find(']', offset);
      if(pos != offset + 1)
      {
        // get number
        std::string number_string = line.substr(offset + 1, pos - offset - 1);
        field.array_size = static_cast<uint16_t>(std::stoi(number_string));
      }
    }

    field.field_name = *str_name;
    trimString(field.field_name);

    // update the hash
    if(field_vector == &schema.fields)
    {
      schema.hash = AddFieldToHash(field, schema.hash);
    }

    field_vector->push_back(field);
  }
  if(check_hash && declared_schema != 0 && declared_schema != schema.hash)
  {
    throw std::runtime_error("Error in hash calculation");
  }
  return schema;
}

template <typename NumberCallback>
bool ParseSnapshotRecursive(const TypeField& field,
                            const std::map<std::string, FieldsVector>& types_list,
                            BufferSpan& buffer, const NumberCallback& callback_number,
                            const std::string& prefix)
{
  [[maybe_unused]] uint32_t vect_size = field.array_size;
  if(field.is_vector && field.array_size == 0)
  {
    // dynamic vector
    vect_size = Deserialize<uint32_t>(buffer);
  }

  auto new_prefix =
      (prefix.empty()) ? field.field_name : (prefix + "/" + field.field_name);

  auto doParse = [&](const std::string& var_name) {
    if(field.type != BasicType::OTHER)
    {
      const auto var = DeserializeToVarNumber(field.type, buffer);
      callback_number(var_name, var);
    }
    else
    {
      const FieldsVector& fields = types_list.at(field.type_name);
      for(const auto& sub_field : fields)
      {
        ParseSnapshotRecursive(sub_field, types_list, buffer, callback_number, var_name);
      }
    }
  };

  if(!field.is_vector)
  {
    doParse(new_prefix);
  }
  else
  {
    for(uint32_t a = 0; a < vect_size; a++)
    {
      const auto& name = new_prefix + "[" + std::to_string(a) + "]";
      doParse(name);
    }
  }
  return true;
}

template <typename NumberCallback, typename CustomCallback>
inline bool ParseSnapshot(const Schema& schema, SnapshotView snapshot,
                          const NumberCallback& callback_number,
                          const CustomCallback& callback_custom)
{
  if(schema.hash != snapshot.schema_hash)
  {
    return false;
  }
  BufferSpan buffer = snapshot.payload;

  for(size_t i = 0; i < schema.fields.size(); i++)
  {
    const auto& field = schema.fields[i];
    if(GetBit(snapshot.active_mask, i))
    {
      ParseSnapshotRecursive(field, schema.custom_types, buffer, callback_number, "");
    }
  }
  return true;
}

}  // namespace DataTamerParser
