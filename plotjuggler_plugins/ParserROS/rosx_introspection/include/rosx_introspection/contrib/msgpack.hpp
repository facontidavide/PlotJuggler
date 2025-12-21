#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

namespace msgpack
{

// MessagePack format byte markers
enum class Format : uint8_t
{
  // Nil and Boolean
  NIL = 0xc0,
  FALSE = 0xc2,
  TRUE = 0xc3,

  // Integer formats
  POSITIVE_FIXINT_MASK = 0x00,  // 0x00 - 0x7f
  POSITIVE_FIXINT_MAX = 0x7f,

  NEGATIVE_FIXINT_MASK = 0xe0,  // 0xe0 - 0xff
  NEGATIVE_FIXINT_MIN = 0xe0,

  UINT8 = 0xcc,
  UINT16 = 0xcd,
  UINT32 = 0xce,
  UINT64 = 0xcf,

  INT8 = 0xd0,
  INT16 = 0xd1,
  INT32 = 0xd2,
  INT64 = 0xd3,

  // Float formats
  FLOAT32 = 0xca,
  FLOAT64 = 0xcb,

  // String formats
  FIXSTR_MASK = 0xa0,  // 0xa0 - 0xbf (up to 31 bytes)
  FIXSTR_MAX_LEN = 31,
  STR8 = 0xd9,
  STR16 = 0xda,
  STR32 = 0xdb,

  // Array formats
  FIXARRAY_MASK = 0x90,  // 0x90 - 0x9f (up to 15 elements)
  FIXARRAY_MAX_SIZE = 15,
  ARRAY16 = 0xdc,
  ARRAY32 = 0xdd,

  // Map formats
  FIXMAP_MASK = 0x80,  // 0x80 - 0x8f (up to 15 pairs)
  FIXMAP_MAX_SIZE = 15,
  MAP16 = 0xde,
  MAP32 = 0xdf
};

// Helper to write big-endian values
template <typename T>
inline void write_be(uint8_t* data, T value)
{
  if constexpr (sizeof(T) == 1)
  {
    data[0] = static_cast<uint8_t>(value);
  }
  else if constexpr (sizeof(T) == 2)
  {
    data[0] = static_cast<uint8_t>(value >> 8);
    data[1] = static_cast<uint8_t>(value);
  }
  else if constexpr (sizeof(T) == 4)
  {
    data[0] = static_cast<uint8_t>(value >> 24);
    data[1] = static_cast<uint8_t>(value >> 16);
    data[2] = static_cast<uint8_t>(value >> 8);
    data[3] = static_cast<uint8_t>(value);
  }
  else if constexpr (sizeof(T) == 8)
  {
    data[0] = static_cast<uint8_t>(value >> 56);
    data[1] = static_cast<uint8_t>(value >> 48);
    data[2] = static_cast<uint8_t>(value >> 40);
    data[3] = static_cast<uint8_t>(value >> 32);
    data[4] = static_cast<uint8_t>(value >> 24);
    data[5] = static_cast<uint8_t>(value >> 16);
    data[6] = static_cast<uint8_t>(value >> 8);
    data[7] = static_cast<uint8_t>(value);
  }
  else
  {
    // Fallback for other sizes (shouldn't happen in practice)
    for (size_t i = 0; i < sizeof(T); ++i)
    {
      data[i] = static_cast<uint8_t>(value >> (8 * (sizeof(T) - 1 - i)));
    }
  }
}

// Pack nil
inline uint32_t pack_nil(uint8_t* data)
{
  data[0] = static_cast<uint8_t>(Format::NIL);
  return 1;
}

// Pack boolean
inline uint32_t pack_bool(uint8_t* data, bool value)
{
  data[0] = static_cast<uint8_t>(value ? Format::TRUE : Format::FALSE);
  return 1;
}

// Pack unsigned integers
inline uint32_t pack_uint(uint8_t* data, uint64_t value)
{
  if (value <= static_cast<uint8_t>(Format::POSITIVE_FIXINT_MAX))
  {
    // positive fixint
    data[0] = static_cast<uint8_t>(value);
    return 1;
  }
  else if (value <= 0xff)
  {
    // uint 8
    data[0] = static_cast<uint8_t>(Format::UINT8);
    data[1] = static_cast<uint8_t>(value);
    return 2;
  }
  else if (value <= 0xffff)
  {
    // uint 16
    data[0] = static_cast<uint8_t>(Format::UINT16);
    write_be<uint16_t>(data + 1, static_cast<uint16_t>(value));
    return 3;
  }
  else if (value <= 0xffffffff)
  {
    // uint 32
    data[0] = static_cast<uint8_t>(Format::UINT32);
    write_be<uint32_t>(data + 1, static_cast<uint32_t>(value));
    return 5;
  }
  else
  {
    // uint 64
    data[0] = static_cast<uint8_t>(Format::UINT64);
    write_be<uint64_t>(data + 1, value);
    return 9;
  }
}

// Pack signed integers
inline uint32_t pack_int(uint8_t* data, int64_t value)
{
  if (value >= 0)
  {
    return pack_uint(data, static_cast<uint64_t>(value));
  }

  if (value >= -32)
  {
    // negative fixint
    data[0] = static_cast<uint8_t>(value);
    return 1;
  }
  else if (value >= -128)
  {
    // int 8
    data[0] = static_cast<uint8_t>(Format::INT8);
    data[1] = static_cast<uint8_t>(value);
    return 2;
  }
  else if (value >= -32768)
  {
    // int 16
    data[0] = static_cast<uint8_t>(Format::INT16);
    write_be<int16_t>(data + 1, static_cast<int16_t>(value));
    return 3;
  }
  else if (value >= -2147483648LL)
  {
    // int 32
    data[0] = static_cast<uint8_t>(Format::INT32);
    write_be<int32_t>(data + 1, static_cast<int32_t>(value));
    return 5;
  }
  else
  {
    // int 64
    data[0] = static_cast<uint8_t>(Format::INT64);
    write_be<int64_t>(data + 1, value);
    return 9;
  }
}

// Pack float
inline uint32_t pack_float(uint8_t* data, float value)
{
  data[0] = static_cast<uint8_t>(Format::FLOAT32);
  uint32_t bits;
  std::memcpy(&bits, &value, sizeof(float));
  write_be<uint32_t>(data + 1, bits);
  return 5;
}

// Pack double
inline uint32_t pack_double(uint8_t* data, double value)
{
  data[0] = static_cast<uint8_t>(Format::FLOAT64);
  uint64_t bits;
  std::memcpy(&bits, &value, sizeof(double));
  write_be<uint64_t>(data + 1, bits);
  return 9;
}

inline uint32_t pack_string(uint8_t* data, const char* str, size_t len)
{
  uint32_t offset = 0;

  if (len <= static_cast<uint8_t>(Format::FIXSTR_MAX_LEN))
  {
    // fixstr
    data[offset++] = static_cast<uint8_t>(Format::FIXSTR_MASK) | static_cast<uint8_t>(len);
  }
  else if (len <= 0xff)
  {
    // str 8
    data[offset++] = static_cast<uint8_t>(Format::STR8);
    data[offset++] = static_cast<uint8_t>(len);
  }
  else if (len <= 0xffff)
  {
    // str 16
    data[offset++] = static_cast<uint8_t>(Format::STR16);
    write_be<uint16_t>(data + offset, static_cast<uint16_t>(len));
    offset += 2;
  }
  else
  {
    // str 32
    data[offset++] = static_cast<uint8_t>(Format::STR32);
    write_be<uint32_t>(data + offset, static_cast<uint32_t>(len));
    offset += 4;
  }

  std::memcpy(data + offset, str, len);
  return offset + len;
}

inline uint32_t pack_string(uint8_t* data, const std::string& str)
{
  return pack_string(data, str.c_str(), str.size());
}

// Pack array header (you need to pack elements separately)
inline uint32_t pack_array(uint8_t* data, uint32_t size)
{
  if (size <= static_cast<uint8_t>(Format::FIXARRAY_MAX_SIZE))
  {
    // fixarray
    data[0] = static_cast<uint8_t>(Format::FIXARRAY_MASK) | static_cast<uint8_t>(size);
    return 1;
  }
  else if (size <= 0xffff)
  {
    // array 16
    data[0] = static_cast<uint8_t>(Format::ARRAY16);
    write_be<uint16_t>(data + 1, static_cast<uint16_t>(size));
    return 3;
  }
  else
  {
    // array 32
    data[0] = static_cast<uint8_t>(Format::ARRAY32);
    write_be<uint32_t>(data + 1, size);
    return 5;
  }
}

// Pack map header (you need to pack key-value pairs separately)
inline uint32_t pack_map(uint8_t* data, uint32_t size)
{
  if (size <= static_cast<uint8_t>(Format::FIXMAP_MAX_SIZE))
  {
    // fixmap
    data[0] = static_cast<uint8_t>(Format::FIXMAP_MASK) | static_cast<uint8_t>(size);
    return 1;
  }
  else if (size <= 0xffff)
  {
    // map 16
    data[0] = static_cast<uint8_t>(Format::MAP16);
    write_be<uint16_t>(data + 1, static_cast<uint16_t>(size));
    return 3;
  }
  else
  {
    // map 32
    data[0] = static_cast<uint8_t>(Format::MAP32);
    write_be<uint32_t>(data + 1, size);
    return 5;
  }
}

// Generic template for packing numbers
template <typename T>
inline uint32_t pack_number(uint8_t* data, T value)
{
  if constexpr (std::is_floating_point_v<T>)
  {
    if constexpr (sizeof(T) == 4)
    {
      return pack_float(data, static_cast<float>(value));
    }
    else
    {
      return pack_double(data, static_cast<double>(value));
    }
  }
  else if constexpr (std::is_unsigned_v<T>)
  {
    return pack_uint(data, static_cast<uint64_t>(value));
  }
  else if constexpr (std::is_signed_v<T>)
  {
    return pack_int(data, static_cast<int64_t>(value));
  }
  else if constexpr (std::is_same_v<T, bool>)
  {
    return pack_bool(data, value);
  }
  else
  {
    static_assert(sizeof(T) == 0, "Unsupported type for pack_number");
    return 0;
  }
}

}  // namespace msgpack