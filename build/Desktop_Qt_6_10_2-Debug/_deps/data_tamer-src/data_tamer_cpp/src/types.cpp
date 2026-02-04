#include "data_tamer/types.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <unordered_map>

namespace DataTamer
{

// clang-format off
static const std::array<std::string, TypesCount> kNames = {
    "bool", "char",
    "int8", "uint8",
    "int16", "uint16",
    "int32", "uint32",
    "int64", "uint64",
    "float32", "float64",
    "other"
};
// clang-format on

const std::string& ToStr(const BasicType& type)
{
  return kNames[static_cast<size_t>(type)];
}

BasicType FromStr(const std::string& str)
{
  static const auto kMap = []() {
    std::unordered_map<std::string, BasicType> map;
    for(size_t i = 0; i < TypesCount; i++)
    {
      auto type = static_cast<BasicType>(i);
      map[ToStr(type)] = type;
    }
    return map;
  }();

  auto const it = kMap.find(str);
  return it == kMap.end() ? BasicType::OTHER : it->second;
}

size_t SizeOf(const BasicType& type)
{
  // clang-format off
  static constexpr std::array<size_t, TypesCount> kSizes =
      { 1, 1,
        1, 1,
        2, 2, 4, 4, 8, 8,
        4, 8, 0 };
  // clang-format on
  return kSizes[static_cast<size_t>(type)];
}

template <typename T>
T DeserializeImpl(const void* data)
{
  T var;
  std::memcpy(&var, data, sizeof(T));
  return var;
}

VarNumber DeserializeAsVarType(const BasicType& type, const void* data)
{
  // clang-format off
  switch(type)
  {
    case BasicType::BOOL: return DeserializeImpl<bool>(data);
    case BasicType::CHAR: return DeserializeImpl<char>(data);

    case BasicType::INT8: return DeserializeImpl<int8_t>(data);
    case BasicType::UINT8: return DeserializeImpl<uint8_t>(data);

    case BasicType::INT16: return DeserializeImpl<int16_t>(data);
    case BasicType::UINT16: return DeserializeImpl<uint16_t>(data);

    case BasicType::INT32: return DeserializeImpl<int32_t>(data);
    case BasicType::UINT32: return DeserializeImpl<uint32_t>(data);

    case BasicType::INT64: return DeserializeImpl<int64_t>(data);
    case BasicType::UINT64: return DeserializeImpl<uint64_t>(data);

    case BasicType::FLOAT32: return DeserializeImpl<float>(data);
    case BasicType::FLOAT64: return DeserializeImpl<double>(data);

    case BasicType::OTHER:
      return double(std::numeric_limits<double>::quiet_NaN());
  }
  // clang-format on
  return {};
}

uint64_t AddFieldToHash(const TypeField& field, uint64_t hash)
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

std::ostream& operator<<(std::ostream& os, const TypeField& field)
{
  if(field.type == BasicType::OTHER)
  {
    os << field.type_name;
  }
  else
  {
    os << ToStr(field.type);
  }

  if(field.is_vector && field.array_size != 0)
  {
    os << "[" << field.array_size << "]";
  }
  if(field.is_vector && field.array_size == 0)
  {
    os << "[]";
  }
  os << ' ' << field.field_name;
  return os;
}

std::ostream& operator<<(std::ostream& os, const Schema& schema)
{
  os << "### version: " << SCHEMA_VERSION << "\n";
  os << "### hash: " << schema.hash << "\n";
  os << "### channel_name: " << schema.channel_name << "\n\n";

  //  std::map<std::string, CustomSerializer::Ptr> custom_types;
  for(const auto& field : schema.fields)
  {
    os << field << "\n";
  }

  for(const auto& [type_name, custom_fields] : schema.custom_types)
  {
    os << "===========================================================\n"
       << "MSG: " << type_name << "\n";
    for(const auto& field : custom_fields)
    {
      os << field << "\n";
    }
  }
  for(const auto& [type_name, custom_schema] : schema.custom_schemas)
  {
    os << "===========================================================\n"
       << "MSG: " << type_name << "\n"
       << "ENCODING: " << custom_schema.encoding << "\n"
       << custom_schema.schema << "\n";
  }

  return os;
}

bool TypeField::operator==(const TypeField& other) const
{
  return is_vector == other.is_vector && type == other.type &&
         array_size == other.array_size && field_name == other.field_name &&
         type_name == other.type_name;
}

bool TypeField::operator!=(const TypeField& other) const
{
  return !(*this == other);
}

std::string ToStr(const Schema& schema)
{
  std::ostringstream ss;
  ss << schema;
  return ss.str();
}

}  // namespace DataTamer
