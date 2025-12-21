#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

namespace PJ
{

enum class BuiltinType : uint8_t
{
  UINT8 = 0,
  UINT16 = 1,
  UINT32 = 2,
  UINT64 = 3,
  INT8 = 8,
  INT16 = 9,
  INT32 = 10,
  INT64 = 11,
  FLOAT32 = 16,
  FLOAT64 = 17,
  STRING = 18,
  BLOB = 19,
  BOOL = 20,
  OTHER = 255
};

template <typename T>
constexpr inline BuiltinType get_builtin_type()
{
  if constexpr (std::is_same_v<T, uint8_t>)
  {
    return BuiltinType::UINT8;
  }
  else if constexpr (std::is_same_v<T, uint16_t>)
  {
    return BuiltinType::UINT16;
  }
  else if constexpr (std::is_same_v<T, uint32_t>)
  {
    return BuiltinType::UINT32;
  }
  else if constexpr (std::is_same_v<T, uint64_t>)
  {
    return BuiltinType::UINT64;
  }
  else if constexpr (std::is_same_v<T, int8_t>)
  {
    return BuiltinType::INT8;
  }
  else if constexpr (std::is_same_v<T, int16_t>)
  {
    return BuiltinType::INT16;
  }
  else if constexpr (std::is_same_v<T, int32_t>)
  {
    return BuiltinType::INT32;
  }
  else if constexpr (std::is_same_v<T, int64_t>)
  {
    return BuiltinType::INT64;
  }
  else if constexpr (std::is_same_v<T, float>)
  {
    return BuiltinType::FLOAT32;
  }
  else if constexpr (std::is_same_v<T, double>)
  {
    return BuiltinType::FLOAT64;
  }
  else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
  {
    return BuiltinType::STRING;
  }
  else if constexpr (std::is_same_v<T, std::vector<uint8_t>>)
  {
    return BuiltinType::BLOB;
  }
  else if constexpr (std::is_same_v<T, bool>)
  {
    return BuiltinType::BOOL;
  }
  else
  {
    return BuiltinType::OTHER;
  }
}

// Simple serialization rules (little endian will be used used):

// 1. A string is always represented as a **uint32_t** with the number of characters (N), followed
// by the N character themselves.
// 2. A number is 1 bytes, representing **BuiltinType** followed by the number itself, where the
// amount of bytes is consistent with the type, for instance 2 for `int16_t`, 4 for `float` and 8
// for `double`.
// 3. A blob is represented as 2 consecutive `uint32_t` numbers, being the first the offset in
// memory pointing to the beginning of the blob (with respect to the beginning of the raw message),
// followed by the size of the blob.

// The "output_buffer" will contain a serialized list of key values, encoded as follows
// - first 4 bytes of the **output_buffer** contains the number of key/value pairs.
// - a "key" is always a string, while the "value" could be either a number, string or blob.

template <typename T>
inline uint32_t pack_number(uint8_t* data, const T& value)
{
  static_assert(std::is_arithmetic<T>::value, "Type must be numeric");
  std::memcpy(data, &value, sizeof(T));
  return sizeof(T) + 1;
}

inline uint32_t pack_string(uint8_t* data, std::string_view str)
{
  const uint32_t len = static_cast<uint32_t>(str.size());
  // Write the length as uint32_t
  std::memcpy(data, &len, sizeof(uint32_t));
  // Write the characters
  std::memcpy(data + sizeof(uint32_t), str.data(), len);
  return sizeof(uint32_t) + len;
}

inline uint32_t pack_blob(uint8_t* data, uint32_t offset, uint32_t size)
{
  // Write the offset as uint32_t
  std::memcpy(data, &offset, sizeof(uint32_t));
  // Write the size as uint32_t
  std::memcpy(data + sizeof(uint32_t), &size, sizeof(uint32_t));
  return 2 * sizeof(uint32_t);
}

inline uint32_t unpack_string(const uint8_t* data, std::string_view& str)
{
  uint32_t len = 0;
  std::memcpy(&len, data, sizeof(uint32_t));
  str = std::string_view(reinterpret_cast<const char*>(data + sizeof(uint32_t)), len);
  return sizeof(uint32_t) + len;
}

template <typename T>
inline uint32_t unpack_number(const uint8_t* data, T& value)
{
  static_assert(std::is_arithmetic<T>::value, "Type must be numeric");
  // Read the number itself
  std::memcpy(&value, data, sizeof(T));
  return sizeof(T);
}

uint32_t unpack_number_into_double(const uint8_t* data, BuiltinType type, double& value)
{
  switch (type)
  {
    case BuiltinType::UINT8: {
      uint8_t v = 0;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 1;
    }
    case BuiltinType::UINT16: {
      uint16_t v = 0;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 2;
    }
    case BuiltinType::UINT32: {
      uint32_t v = 0;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 4;
    }
    case BuiltinType::UINT64: {
      uint64_t v = 0;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 8;
    }
    case BuiltinType::INT8: {
      int8_t v = 0;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 1;
    }
    case BuiltinType::INT16: {
      int16_t v = 0;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 2;
    }
    case BuiltinType::INT32: {
      int32_t v = 0;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 4;
    }
    case BuiltinType::INT64: {
      int64_t v = 0;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 8;
    }
    case BuiltinType::FLOAT32: {
      float v = 0.0f;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 4;
    }
    case BuiltinType::FLOAT64: {
      double v = 0.0;
      unpack_number(data, v);
      value = v;
      return 8;
    }
    case BuiltinType::BOOL: {
      uint8_t v = 0;
      unpack_number(data, v);
      value = static_cast<double>(v);
      return 1;
    }
    default:
      throw std::runtime_error("Unsupported BuiltinType for unpack_number_into_double");
  }
}

}  // namespace PJ
