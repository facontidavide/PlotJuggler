#pragma once

#include <cstdint>
#include <memory>

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

namespace DataTamer
{

constexpr int SCHEMA_VERSION = 4;

// clang-format off
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


using VarNumber = std::variant<
    bool, char,
    int8_t, uint8_t,
    int16_t, uint16_t,
    int32_t, uint32_t,
    int64_t, uint64_t,
    float, double >;
// clang-format on

/// Reverse operation of ValuePtr::serialize
[[nodiscard]] VarNumber DeserializeAsVarType(const BasicType& type, const void* data);

/// Return the number of bytes needed to serialize the type
[[nodiscard]] size_t SizeOf(const BasicType& type);

/// Return the name of the type
[[nodiscard]] const std::string& ToStr(const BasicType& type);

/// Convert string to its type
[[nodiscard]] BasicType FromStr(const std::string& str);

template <typename T>
inline constexpr BasicType GetBasicType()
{
  if constexpr(std::is_enum_v<T>)
  {
    return GetBasicType<std::underlying_type_t<T>>();
  }

  // clang-format off
  if constexpr (std::is_same_v<T, bool>) return BasicType::BOOL;
  if constexpr (std::is_same_v<T, char>) return BasicType::CHAR;
  if constexpr (std::is_same_v<T, std::int8_t>) return BasicType::INT8;
  if constexpr (std::is_same_v<T, std::uint8_t>) return BasicType::UINT8;

  if constexpr (std::is_same_v<T, std::int16_t>) return BasicType::INT16;
  if constexpr (std::is_same_v<T, std::uint16_t>) return BasicType::UINT16;

  if constexpr (std::is_same_v<T, std::int32_t>) return BasicType::INT32;
  if constexpr (std::is_same_v<T, std::uint32_t>) return BasicType::UINT32;

  if constexpr (std::is_same_v<T, std::uint64_t>) return BasicType::UINT64;
  if constexpr (std::is_same_v<T, std::int64_t>) return BasicType::INT64;

  if constexpr (std::is_same_v<T, float>) return BasicType::FLOAT32;
  if constexpr (std::is_same_v<T, double>) return BasicType::FLOAT64;
  // clang-format on
  return BasicType::OTHER;
}

template <typename T>
inline constexpr bool IsNumericType()
{
  return std::is_arithmetic_v<T> || std::is_same_v<T, bool> || std::is_same_v<T, char> ||
         std::is_enum_v<T>;
}

struct RegistrationID
{
  size_t first_index = 0;
  size_t fields_count = 0;

  // syntactic sugar to be used to concatenate contiguous RegistrationID.
  void operator+=(const RegistrationID& other) { fields_count += other.fields_count; }
};

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

  friend std::ostream& operator<<(std::ostream& os, const TypeField& field);
};

using FieldsVector = std::vector<TypeField>;
class CustomSerializer;

struct CustomSchema
{
  std::string encoding;
  std::string schema;
};

/**
 * @brief DataTamer uses a simple "flat" schema of key/value pairs (each pair is a "field").
 */
struct Schema
{
  uint64_t hash = 0;
  FieldsVector fields;
  std::string channel_name;

  std::unordered_map<std::string, FieldsVector> custom_types;
  std::unordered_map<std::string, CustomSchema> custom_schemas;

  friend std::ostream& operator<<(std::ostream& os, const Schema& schema);
};

std::string ToStr(const Schema& schema);

[[nodiscard]] uint64_t AddFieldToHash(const TypeField& field, uint64_t hash);

}  // namespace DataTamer

template <>
struct std::hash<DataTamer::RegistrationID>
{
  std::size_t operator()(const DataTamer::RegistrationID& id) const
  {
    // Compute individual hash values for first, second and third
    // http://stackoverflow.com/a/1646913/126995
    std::size_t res = 17;
    res = res * 31 + hash<size_t>()(id.first_index);
    res = res * 31 + hash<size_t>()(id.fields_count);
    return res;
  }
};
