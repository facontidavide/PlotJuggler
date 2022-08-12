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

#ifndef ROS_INTROSPECTION_ROSTYPE_H
#define ROS_INTROSPECTION_ROSTYPE_H

#include <vector>
#include <map>
#include <iostream>
#include <functional>
#include "rosx_introspection/variant.hpp"

namespace RosMsgParser{

/**
 * @brief ROSType
 */
class ROSType {
public:

  ROSType(){}

  ROSType(const std::string& name);

  ROSType(const ROSType& other) {  *this = other; }

  ROSType(ROSType&& other) {  *this = other; }

  ROSType& operator= (const ROSType& other);

  ROSType& operator= (ROSType&& other);

  /// Concatenation of msg_name and pkg_name.
  /// ex.: geometry_msgs/Pose"
  const std::string& baseName() const;

  /// ex.: geometry_msgs/Pose -> "Pose"
  const std::string_view& msgName()  const;

  /// ex.: geometry_msgs/Pose -> "geometry_msgs"
  const std::string_view& pkgName()  const;

  void setPkgName(std::string_view new_pkg);

  /// True if the type is ROS builtin
  bool isBuiltin() const;

  /// If builtin, size of builtin, -1 means variable or undefined
  int typeSize() const;

  /// If type is builtin, returns the id.  BuiltinType::OTHER otherwise.
  BuiltinType typeID() const;

  bool operator==(const ROSType& other) const  {
    return _hash == other._hash;
  }

  bool operator!=(const ROSType& other) const  {
    return (_hash != other._hash);
  }

  bool operator<(const ROSType& other) const {
    return this->baseName() < other.baseName();
  }

  size_t hash() const { return _hash; }

protected:

  BuiltinType _id = OTHER;
  std::string _base_name;
  std::string_view _msg_name;
  std::string_view _pkg_name;
  size_t _hash = 0;

};

//----------- definitions -------------

inline const std::string &ROSType::baseName() const
{
  return _base_name;
}

inline const std::string_view& ROSType::msgName() const
{
  return _msg_name;
}

inline const std::string_view &ROSType::pkgName() const
{
  return _pkg_name;
}

inline bool ROSType::isBuiltin() const
{
  return _id != RosMsgParser::OTHER;
}

inline int ROSType::typeSize() const
{
  return builtinSize( _id );
}

inline BuiltinType ROSType::typeID() const
{
  return _id;
}

//--------- helper functions --------------

inline std::ostream& operator<<(std::ostream &os, const ROSType& t )
{
  os << t.baseName();
  return os;
}

inline BuiltinType toBuiltinType(const std::string_view& s) {
  static std::map<std::string_view, BuiltinType> string_to_builtin_map {
    { "bool", BOOL },
    { "byte", BYTE },
    { "char", CHAR },
    { "uint8", UINT8 },
    { "uint16", UINT16 },
    { "uint32", UINT32 },
    { "uint64", UINT64 },
    { "int8", INT8 },
    { "int16", INT16 },
    { "int32", INT32 },
    { "int64", INT64 },
    { "float32", FLOAT32 },
    { "float64", FLOAT64 },
    { "time", TIME },
    { "duration", DURATION },
    { "string", STRING },
  };
  const auto it = string_to_builtin_map.find(s);
  return (it != string_to_builtin_map.cend()) ? it->second : OTHER;
}

}

namespace std {
  template <> struct hash<RosMsgParser::ROSType>
  {

    typedef RosMsgParser::ROSType argument_type;
    typedef std::size_t               result_type;

    result_type operator()(RosMsgParser::ROSType const& type) const
    {
      return type.hash();
    }
  };
}


#endif // ROSTYPE_H
