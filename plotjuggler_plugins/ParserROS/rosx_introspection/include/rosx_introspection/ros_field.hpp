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

#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "rosx_introspection/ros_type.hpp"

namespace RosMsgParser {

class ROSMessage;
struct EnumDefinition;
struct DiscriminatedUnion;

using RosMessageLibrary = std::unordered_map<ROSType, std::shared_ptr<ROSMessage>>;

class Parser;

/**
 * @brief A ROSMessage will contain one or more ROSField(s). Each field is little more
 * than a name / type pair.
 */
class ROSField {
 public:
  ROSField(const ROSType& type, const std::string& name);

  ROSField(const std::string& definition);

  const std::string& name() const {
    return _fieldname;
  }

  const ROSType& type() const {
    return _type;
  }

  void changeType(const ROSType& type) {
    _type = type;
  }

  /// True if field is a constant in message definition
  bool isConstant() const {
    return _is_constant;
  }

  /// If constant, value of field, else undefined
  const std::string& value() const {
    return _value;
  }

  /// True if the type is an array
  bool isArray() const {
    return _is_array;
  }

  /// 1 if !is_array, -1 if is_array and array is
  /// variable length, otherwise length in name
  int arraySize() const {
    return _array_size;
  }

  void setArray(bool is_array, int size) {
    _is_array = is_array;
    _array_size = size;
  }

  /// For multi-dimensional arrays: stores each dimension separately.
  /// E.g., data[3][4] -> {3, 4}. Empty for 1D arrays or non-arrays.
  const SmallVector<int, 2>& arrayDimensions() const {
    return _array_dims;
  }

  void setArrayDimensions(const SmallVector<int, 2>& dims) {
    _array_dims = dims;
  }

  bool isOptional() const {
    return _is_optional;
  }

  void setOptional(bool optional) {
    _is_optional = optional;
  }

  bool isKey() const {
    return _is_key;
  }

  void setIsKey(bool key) {
    _is_key = key;
  }

  const EnumDefinition* getEnum() const {
    return _enum_ptr;
  }

  void setEnumPtr(const EnumDefinition* ptr) {
    _enum_ptr = ptr;
  }

  const DiscriminatedUnion* getUnion() const {
    return _union_ptr;
  }

  void setUnionPtr(const DiscriminatedUnion* ptr) {
    _union_ptr = ptr;
  }

  friend class ROSMessage;

  std::shared_ptr<ROSMessage> getMessagePtr(const RosMessageLibrary& library) const;

 protected:
  std::string _fieldname;
  ROSType _type;
  std::string _value;
  bool _is_array;
  bool _is_constant = false;
  int _array_size;
  SmallVector<int, 2> _array_dims;
  bool _is_optional = false;
  bool _is_key = false;

  const EnumDefinition* _enum_ptr = nullptr;
  const DiscriminatedUnion* _union_ptr = nullptr;

  mutable const RosMessageLibrary* _cache_library = nullptr;
  mutable std::shared_ptr<ROSMessage> _cache_message;
};

void TrimStringLeft(std::string& s);

void TrimStringRight(std::string& s);

void TrimString(std::string& s);

}  // namespace RosMsgParser
