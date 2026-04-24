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

#include "rosx_introspection/ros_type.hpp"

#include <assert.h>

namespace RosMsgParser {

namespace {
// Locate the package/message boundary in a type name.
// Accepts both ROS-style "pkg/Type" and IDL-style "pkg::Type" (and nested
// modules like "a::b::Type"). IDL takes priority: if "::" is present the last
// occurrence is the separator, otherwise the first "/" is used.
// Returns {pkg_len, msg_start}; when no separator is found both are 0 and the
// whole name is treated as the message name.
std::pair<size_t, size_t> splitTypeName(const std::string& name) {
  auto colon = name.rfind("::");
  if (colon != std::string::npos) {
    return {colon, colon + 2};
  }
  auto slash = name.find('/');
  if (slash != std::string::npos) {
    return {slash, slash + 1};
  }
  return {0, 0};
}
}  // namespace

ROSType::ROSType(const std::string& name) : _base_name(name) {
  auto [pkg_len, msg_start] = splitTypeName(_base_name);
  _pkg_name = std::string_view(_base_name.data(), pkg_len);
  _msg_name = std::string_view(_base_name.data() + msg_start, _base_name.size() - msg_start);

  _id = toBuiltinType(_msg_name);
  _hash = std::hash<std::string>{}(_base_name);
}

ROSType& ROSType::operator=(const ROSType& other) {
  _base_name = other._base_name;
  auto [pkg_len, msg_start] = splitTypeName(_base_name);
  _pkg_name = std::string_view(_base_name.data(), pkg_len);
  _msg_name = std::string_view(_base_name.data() + msg_start, _base_name.size() - msg_start);
  _id = other._id;
  _hash = other._hash;
  return *this;
}

ROSType& ROSType::operator=(ROSType&& other) {
  _base_name = std::move(other._base_name);
  auto [pkg_len, msg_start] = splitTypeName(_base_name);
  _pkg_name = std::string_view(_base_name.data(), pkg_len);
  _msg_name = std::string_view(_base_name.data() + msg_start, _base_name.size() - msg_start);
  _id = other._id;
  _hash = other._hash;
  return *this;
}

void ROSType::setPkgName(std::string_view new_pkg) {
  assert(_pkg_name.size() == 0);

  size_t pos = new_pkg.size();
  _base_name = std::string(new_pkg) + "/" + _base_name;

  _pkg_name = std::string_view(_base_name.data(), pos++);
  _msg_name = std::string_view(_base_name.data() + pos, _base_name.size() - pos);

  _hash = std::hash<std::string>{}(_base_name);
}

}  // namespace RosMsgParser
