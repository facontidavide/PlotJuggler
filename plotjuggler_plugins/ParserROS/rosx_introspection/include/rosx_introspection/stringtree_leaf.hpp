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

#pragma once

#include <cstring>
#include <iostream>
#include <map>
#include <vector>

#include "rosx_introspection/ros_message.hpp"

// Brutally faster for numbers below 100
inline int print_number(char* buffer, uint16_t value) {
  const char DIGITS[] =
      "00010203040506070809"
      "10111213141516171819"
      "20212223242526272829"
      "30313233343536373839"
      "40414243444546474849"
      "50515253545556575859"
      "60616263646566676869"
      "70717273747576777879"
      "80818283848586878889"
      "90919293949596979899";
  if (value < 10) {
    buffer[0] = static_cast<char>('0' + value);
    return 1;
  } else if (value < 100) {
    value *= 2;
    buffer[0] = DIGITS[value];
    buffer[1] = DIGITS[value + 1];
    return 2;
  } else {
    return snprintf(buffer, 16, "%d", value);
  }
}

namespace RosMsgParser {

/// Fixed-size buffer for @key suffixes (e.g., "[ArmID:3]").
/// Avoids heap allocation — typical key suffixes are < 48 chars.
struct KeySuffix {
  char data[48] = {};
  uint8_t len = 0;

  void clear() {
    len = 0;
  }

  bool empty() const {
    return len == 0;
  }

  void assign(const char* src, size_t n) {
    len = static_cast<uint8_t>(n < sizeof(data) ? n : sizeof(data) - 1);
    std::memcpy(data, src, len);
    data[len] = '\0';
  }
};

struct KeySuffixAtDepth {
  uint16_t depth = 0;
  KeySuffix suffix;
};

/**
 * @brief The FieldTreeLeaf is, as the name suggests, a leaf (terminal node)
 * of a StringTree.
 * It provides the pointer to the node and a list of numbers that represent
 * the index that corresponds to the placeholder "#".
 */
struct FieldLeaf {
  const FieldTreeNode* node = nullptr;
  SmallVector<uint16_t, 4> index_array;
  SmallVector<uint16_t, 4> index_depth_array;
  SmallVector<KeySuffixAtDepth, 4> key_suffixes;
  KeySuffix key_suffix;

  /// Convert to human-readable path string (e.g., "topic[ArmID:3]/field[J1]/value")
  void toStr(std::string& out) const;

  std::string toStdString() const {
    std::string out;
    toStr(out);
    return out;
  }
};

// Keep FieldsVector for backward compatibility
struct FieldsVector {
  FieldsVector() = default;

  FieldsVector(const FieldLeaf& leaf);

  SmallVector<uint16_t, 4> index_array;
  SmallVector<uint16_t, 4> index_depth_array;
  SmallVector<KeySuffixAtDepth, 4> key_suffixes;
  KeySuffix key_suffix;

  void toStr(std::string& destination) const;

  std::string toStdString() const {
    std::string out;
    toStr(out);
    return out;
  }

 private:
  const FieldTreeNode* _node = nullptr;
};

//---------------------------------

inline std::ostream& operator<<(std::ostream& os, const FieldLeaf& leaf) {
  std::string dest;
  leaf.toStr(dest);
  os << dest;
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const FieldsVector& leaf) {
  std::string dest;
  leaf.toStr(dest);
  os << dest;
  return os;
}

}  // namespace RosMsgParser
