/***** MIT License ****
*
*   Copyright (c) 2016-2022 Davide Faconti
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*    copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*/


#ifndef ROS_BUILTIN_TYPES_HPP
#define ROS_BUILTIN_TYPES_HPP

#include <stdint.h>
#include <string>
#include <iostream>
#include <unordered_map>

#include "rosx_introspection/contrib/span.hpp"
#include "rosx_introspection/contrib/SmallVector.h"

namespace RosMsgParser{

template< class T>
using Span = nonstd::span<T>;

template< class T, size_t N>
using SmallVector = llvm_vecsmall::SmallVector<T, N>;

enum BuiltinType {
  BOOL , BYTE, CHAR,
  UINT8, UINT16, UINT32, UINT64,
  INT8, INT16, INT32, INT64,
  FLOAT32, FLOAT64,
  TIME, DURATION,
  STRING, OTHER
};

//---------------------------------------------------------

inline int builtinSize(const BuiltinType c) {
  switch (c) {
  case BOOL:
  case BYTE:
  case INT8:
  case CHAR:
  case UINT8:    return 1;
  case UINT16:
  case INT16:    return 2;
  case UINT32:
  case INT32:
  case FLOAT32:  return 4;
  case UINT64:
  case INT64:
  case FLOAT64:
  case TIME:
  case DURATION: return 8;
  case STRING:
  case OTHER: return -1;
  }
  throw std::runtime_error( "unsupported builtin type value");
}

inline const char* toStr(const BuiltinType& c)
{
  switch (c) {
  case BOOL:     return "BOOL";
  case BYTE:     return "BYTE";
  case INT8:     return "INT8";
  case CHAR:     return "CHAR";
  case UINT8:    return "UINT8";
  case UINT16:   return "UINT16";
  case UINT32:   return "UINT32";
  case UINT64:   return "UINT64";
  case INT16:    return "INT16";
  case INT32:    return "INT32";
  case INT64:    return "INT64";
  case FLOAT32:  return "FLOAT32";
  case FLOAT64:  return "FLOAT64";
  case TIME:     return "TIME";
  case DURATION: return "DURATION";
  case STRING:   return "STRING";
  case OTHER:    return "OTHER";
  }
  throw std::runtime_error( "unsupported builtin type value");
}

inline std::ostream& operator<<(std::ostream& os, const BuiltinType& c)
{
  os << toStr(c);
  return os;
}

template <typename T> BuiltinType getType()
{
    return OTHER;
}

struct Time
{
  uint32_t sec;
  uint32_t nsec;

  double toSec()
  {
    return double(sec) + double(nsec)*1e-9;
  }
};


template <> inline BuiltinType getType<bool>()  {  return BOOL; }

template <> inline BuiltinType getType<char>()           {  return CHAR; }

template <> inline BuiltinType getType<int8_t>()  {  return INT8; }
template <> inline BuiltinType getType<int16_t>() {  return INT16; }
template <> inline BuiltinType getType<int32_t>() {  return INT32; }
template <> inline BuiltinType getType<int64_t>() {  return INT64; }

template <> inline BuiltinType getType<uint8_t>()  {  return UINT8; }
template <> inline BuiltinType getType<uint16_t>() {  return UINT16; }
template <> inline BuiltinType getType<uint32_t>() {  return UINT32; }
template <> inline BuiltinType getType<uint64_t>() {  return UINT64; }

template <> inline BuiltinType getType<float>()  {  return FLOAT32; }
template <> inline BuiltinType getType<double>() {  return FLOAT64; }

template <> inline BuiltinType getType<std::string>() {  return STRING; }

template <> inline BuiltinType getType<RosMsgParser::Time>()     {  return TIME; }

}

#endif // ROS_BUILTIN_TYPES_HPP
