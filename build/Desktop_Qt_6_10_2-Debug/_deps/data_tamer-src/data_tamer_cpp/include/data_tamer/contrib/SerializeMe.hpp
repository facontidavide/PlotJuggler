/**
 * MIT License
 *
 * Copyright (c) 2019-2024 Davide Faconti
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace SerializeMe
{

// Poor man version of Span
template <typename T>
class Span
{
public:
  Span() = default;

  Span(T* ptr, size_t size) : data_(ptr), size_(size) {}

  template <size_t N>
  Span(std::array<T, N>& v) : data_(v.data()), size_(N)
  {}

  Span(std::vector<T>& v) : data_(v.data()), size_(v.size()) {}

  T const* data() const;

  T* data();

  size_t size() const;

  void trimFront(size_t offset);

private:
  T* data_ = nullptr;
  size_t size_ = 0;
};

using SpanBytes = Span<uint8_t>;
using SpanBytesConst = Span<uint8_t const>;
using StringSize = uint16_t;

//------------- Forward declarations of BufferSize ------------------

template <typename T>
size_t BufferSize(const T& val);

template <>
size_t BufferSize(const std::string& str);

template <class T, size_t N>
size_t BufferSize(const std::array<T, N>& v);

template <template <class, class> class Container, class T, class... TArgs>
size_t BufferSize(const Container<T, TArgs...>& vect);

//---------- Forward declarations of DeserializeFromBuffer -----------

template <typename T>
void DeserializeFromBuffer(SpanBytesConst& buffer, T& dest);

template <>
void DeserializeFromBuffer(SpanBytesConst& buffer, std::string& str);

template <class T, size_t N>
void DeserializeFromBuffer(SpanBytesConst& buffer, std::array<T, N>& v);

template <template <class, class> class Container, class T, class... TArgs>
void DeserializeFromBuffer(SpanBytesConst& buffer, Container<T, TArgs...>& dest);

//---------- Forward declarations of SerializeIntoBuffer -----------

template <typename T>
void SerializeIntoBuffer(SpanBytes& buffer, const T& value);

template <>
void SerializeIntoBuffer(SpanBytes& buffer, const std::string& str);

template <class T, size_t N>
void SerializeIntoBuffer(SpanBytes& buffer, const std::array<T, N>& v);

template <template <class, class> class Container, class T, class... TArgs>
void SerializeIntoBuffer(SpanBytes& buffer, const Container<T, TArgs...>& vect);

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

template <typename T>
inline T const* Span<T>::data() const
{
  return data_;
}

template <typename T>
inline T* Span<T>::data()
{
  return data_;
}

template <typename T>
inline size_t Span<T>::size() const
{
  return size_;
}

template <typename T>
inline void Span<T>::trimFront(size_t offset)
{
  if(offset > size_)
  {
    throw std::runtime_error("Buffer overrun");
  }
  size_ -= offset;
  data_ += offset;
}

// The wire format uses a little endian encoding (since that's efficient for
// the common platforms).
#if defined(__s390x__)
#define SERIALIZE_LITTLEENDIAN 0
#endif  // __s390x__
#if !defined(SERIALIZE_LITTLEENDIAN)
#if defined(__GNUC__) || defined(__clang__) || defined(__ICCARM__)
#if(defined(__BIG_ENDIAN__) ||                                                           \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#define SERIALIZE_LITTLEENDIAN 0
#else
#define SERIALIZE_LITTLEENDIAN 1
#endif  // __BIG_ENDIAN__
#elif defined(_MSC_VER)
#if defined(_M_PPC)
#define SERIALIZE_LITTLEENDIAN 0
#else
#define SERIALIZE_LITTLEENDIAN 1
#endif
#else
#error Unable to determine endianness, define SERIALIZE_LITTLEENDIAN.
#endif
#endif  // !defined(SERIALIZE_LITTLEENDIAN)

template <typename T>
inline T EndianSwap(T t)
{
  static_assert(std::is_arithmetic<T>::value, "This function accepts only numeric types");
#if defined(_MSC_VER)
#define DESERIALIZE_ME_BYTESWAP16 _byteswap_ushort
#define DESERIALIZE_ME_BYTESWAP32 _byteswap_ulong
#define DESERIALIZE_ME_BYTESWAP64 _byteswap_uint64
#else
#if defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ < 408 && !defined(__clang__)
// __builtin_bswap16 was missing prior to GCC 4.8.
#define DESERIALIZE_ME_BYTESWAP16(x)                                                     \
  static_cast<uint16_t>(__builtin_bswap32(static_cast<uint32_t>(x) << 16))
#else
#define DESERIALIZE_ME_BYTESWAP16 __builtin_bswap16
#endif
#define DESERIALIZE_ME_BYTESWAP32 __builtin_bswap32
#define DESERIALIZE_ME_BYTESWAP64 __builtin_bswap64
#endif
  if constexpr(sizeof(T) == 1)
  {  // Compile-time if-then's.
    return t;
  }
  else if constexpr(sizeof(T) == 2)
  {
    union
    {
      T t;
      uint16_t i;
    } u;
    u.t = t;
    u.i = DESERIALIZE_ME_BYTESWAP16(u.i);
    return u.t;
  }
  else if constexpr(sizeof(T) == 4)
  {
    union
    {
      T t;
      uint32_t i;
    } u;
    u.t = t;
    u.i = DESERIALIZE_ME_BYTESWAP32(u.i);
    return u.t;
  }
  else if(sizeof(T) == 8)
  {
    union
    {
      T t;
      uint64_t i;
    } u;
    u.t = t;
    u.i = DESERIALIZE_ME_BYTESWAP64(u.i);
    return u.t;
  }
  else
  {
    std::runtime_error("Problem with IndianSwap");
  }
}

// Check if a Function like this is implemented:
//
// template <typename Func> std::string_view TypeDefinition(T&, Func&);

template <typename T, class = void>
struct has_TypeDefinition : std::false_type
{
};

const auto EmptyFuncion = [](const char*, void*) {};
using EmptyFunc = decltype(EmptyFuncion);

template <typename T1, typename T2>
using enable_if_same_t = std::enable_if_t<std::is_same_v<T1, T2>>;

template <typename T>
struct has_TypeDefinition<
    T, enable_if_same_t<std::string_view,
                        decltype(TypeDefinition(std::declval<T&>(),
                                                std::declval<EmptyFunc&>()))>>
  : std::true_type
{
};

template <typename T>
inline constexpr bool is_number()
{
  return std::is_arithmetic_v<T> || std::is_same_v<T, std::byte> || std::is_enum_v<T>;
}

template <typename _Tp, bool _is_container, int _size>
struct container_info_
{
  static constexpr bool is_container = _is_container;
  static constexpr int size = _size;
  typedef _Tp value_type;
};

template <typename T>
struct container_info : container_info_<T, false, -1>
{
};

template <template <class, class> class Container, class T, class... TArgs>
struct container_info<Container<T, TArgs...>> : container_info_<T, true, 0>
{
};

template <typename T, size_t S>
struct container_info<std::array<T, S>> : container_info_<T, true, int(S)>
{
};

template <typename>
struct is_std_vector : std::false_type
{
};

template <typename T, typename... TArgs>
struct is_std_vector<std::vector<T, TArgs...>> : std::true_type
{
};

template <typename>
struct is_std_array : std::false_type
{
  const static size_t Size = 0;
};

template <typename T, size_t S>
struct is_std_array<std::array<T, S>> : std::true_type
{
  const static size_t Size = S;
};

template <typename T>
inline constexpr bool is_vector()
{
  return (is_std_vector<T>::value || is_std_array<T>::value);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

template <typename T>
inline size_t BufferSize(const T& val)
{
  static_assert(is_number<T>() || has_TypeDefinition<T>(), "Missing TypeDefinition");

  if constexpr(is_number<T>())
  {
    return sizeof(T);
  }
  else
  {
    size_t total_size = 0;
    auto func = [&total_size](const char*, auto const* field) {
      total_size += BufferSize(*field);
    };

    TypeDefinition(const_cast<T&>(val), func);
    return total_size;
  }
}

template <>
inline size_t BufferSize(const std::string& str)
{
  return sizeof(StringSize) + str.size();
}

template <class T, size_t N>
inline size_t BufferSize(const std::array<T, N>&)
{
  return BufferSize(T{}) * N;
}

template <template <class, class> class Container, class T, class... TArgs>
inline size_t BufferSize(const Container<T, TArgs...>& vect)
{
  if constexpr(std::is_trivially_copyable_v<T> && is_vector<Container<T, TArgs...>>())
  {
    return sizeof(uint32_t) + vect.size() * sizeof(T);
  }
  else
  {
    auto size = sizeof(uint32_t);
    for(const auto& v : vect)
    {
      size += BufferSize(v);
    }
    return size;
  }
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

template <typename T>
inline void DeserializeFromBuffer(SpanBytesConst& buffer, T& dest)
{
  static_assert(is_number<T>() || has_TypeDefinition<T>(), "Missing TypeDefinition");

  if constexpr(is_number<T>())
  {
    auto const S = sizeof(T);
    if(S > buffer.size())
    {
      throw std::runtime_error("DeserializeFromBuffer: buffer overflow");
    }
    dest = *(reinterpret_cast<T const*>(buffer.data()));

#if SERIALIZE_LITTLEENDIAN == 0
    dest = EndianSwap<T>(dest);
#endif
    buffer = SpanBytesConst(buffer.data() + S, buffer.size() - S);  // NOLINT
  }
  else
  {
    auto func = [&buffer](const char*, const auto* field) {
      DeserializeFromBuffer(buffer, *field);
    };
    TypeDefinition(dest, func);
  }
}

template <>
inline void DeserializeFromBuffer(SpanBytesConst& buffer, std::string& dest)
{
  StringSize size = 0;
  DeserializeFromBuffer(buffer, size);

  if(size > buffer.size())
  {
    throw std::runtime_error("DeserializeFromBuffer: buffer overflow");
  }

  dest.assign(reinterpret_cast<char const*>(buffer.data()), size);
  buffer.trimFront(size);
}

template <typename T, size_t N>
inline void DeserializeFromBuffer(SpanBytesConst& buffer, std::array<T, N>& dest)
{
  if(N * BufferSize(T{}) > buffer.size())
  {
    throw std::runtime_error("DeserializeFromBuffer: buffer overflow");
  }

  if constexpr(sizeof(T) == 1)
  {
    memcpy(dest.data(), buffer.data(), N);
    buffer.trimFront(N);
  }
  else
  {
    for(size_t i = 0; i < N; i++)
    {
      DeserializeFromBuffer(buffer, dest[i]);
    }
  }
}

template <template <class, class> class Container, class T, class... TArgs>
inline void DeserializeFromBuffer(SpanBytesConst& buffer, Container<T, TArgs...>& dest)
{
  uint32_t num_values = 0;
  DeserializeFromBuffer(buffer, num_values);

  // if the container offers contiguous memory, you can just use memcpy
  if constexpr(sizeof(T) == 1 && is_vector<Container<T, TArgs...>>())
  {
    if constexpr(container_info<Container<T, TArgs...>>::size == 0)
    {
      dest.resize(num_values);
    }
    else if constexpr(std::is_array_v<Container<T, TArgs...>>)
    {
      if(std::size(dest) != num_values)
      {
        throw std::runtime_error("DeserializeFromBuffer: wrong size in static container");
      }
    }

    const size_t size = num_values * BufferSize(T{});
    memcpy(dest.data(), buffer.data(), size);
    buffer.trimFront(size);
  }
  else
  {
    dest.clear();
    for(size_t i = 0; i < num_values; i++)
    {
      T temp;
      DeserializeFromBuffer(buffer, temp);
      std::back_inserter(dest) = std::move(temp);
    }
  }
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

template <typename T>
inline void SerializeIntoBuffer(SpanBytes& buffer, T const& value)
{
  static_assert(is_number<T>() || has_TypeDefinition<T>(), "Missing TypeDefinition");

  if constexpr(is_number<T>())
  {
    const size_t S = sizeof(T);
    if(S > buffer.size())
    {
      throw std::runtime_error("SerializeIntoBuffer: buffer overflow");
    }
#if SERIALIZE_LITTLEENDIAN == 0
    *(reinterpret_cast<T*>(buffer.data())) = EndianSwap<T>(value);
#else
    *(reinterpret_cast<T*>(buffer.data())) = value;
#endif
    buffer.trimFront(S);  // NOLINT
  }
  else
  {
    auto func = [&buffer](const char*, const auto* field) {
      SerializeIntoBuffer(buffer, *field);
    };
    TypeDefinition(const_cast<T&>(value), func);
  }
}

template <>
inline void SerializeIntoBuffer(SpanBytes& buffer, std::string const& str)
{
  if(str.size() > std::numeric_limits<StringSize>::max())
  {
    throw std::runtime_error("SerializeIntoBuffer: string exceeds maximum size");
  }

  if((str.size() + sizeof(StringSize)) > buffer.size())
  {
    throw std::runtime_error("SerializeIntoBuffer: buffer overflow");
  }

  const auto size = static_cast<StringSize>(str.size());
  SerializeIntoBuffer(buffer, size);

  memcpy(buffer.data(), str.data(), size);
  buffer.trimFront(size);
}

template <typename T, size_t N>
inline void SerializeIntoBuffer(SpanBytes& buffer, std::array<T, N> const& vect)
{
  if(N > std::numeric_limits<uint32_t>::max())
  {
    throw std::runtime_error("SerializeIntoBuffer: array exceeds maximum size");
  }

  if constexpr(std::is_arithmetic_v<T> || std::is_same_v<T, std::byte>)
  {
    if(N * sizeof(T) > buffer.size())
    {
      throw std::runtime_error("SerializeIntoBuffer: buffer overflow");
    }
  }

  if constexpr(sizeof(T) == 1)
  {
    std::memcpy(vect.data(), buffer.data(), N);
    buffer.trimFront(N);
  }
  else
  {
    for(size_t i = 0; i < N; i++)
    {
      SerializeIntoBuffer(buffer, vect[i]);
    }
  }
}

template <template <class, class> class Container, class T, class... TArgs>
inline void SerializeIntoBuffer(SpanBytes& buffer, Container<T, TArgs...> const& vect)
{
  const auto num_values = static_cast<uint32_t>(vect.size());
  SerializeIntoBuffer(buffer, num_values);

  // can use memcpy if the size of T is 1
  if constexpr(sizeof(T) == 1 && is_vector<Container<T, TArgs...>>())
  {
    const size_t size = num_values;
    if(size > buffer.size())
    {
      throw std::runtime_error("SerializeIntoBuffer: buffer overflow");
    }
    memcpy(buffer.data(), vect.data(), size);
    buffer.trimFront(size);
  }
  else
  {
    for(const T& v : vect)
    {
      SerializeIntoBuffer(buffer, v);
    }
  }
}

}  // namespace SerializeMe
