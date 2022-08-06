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

#include <algorithm>
#include <regex>
#include "rosx_introspection/ros_field.hpp"
#include "rosx_introspection/ros_message.hpp"

namespace RosMsgParser{


ROSField::ROSField(const ROSType &type, const std::string &name):
  _fieldname(name), _type(type), _is_array(false), _array_size(1)
{

}

ROSField::ROSField(const std::string &definition):
  _is_array(false), _array_size(1)
{
  static const  std::regex type_regex("[a-zA-Z][a-zA-Z0-9_]*"
                                       "(/[a-zA-Z][a-zA-Z0-9_]*){0,1}"
                                       "(\\[[0-9]*\\]){0,1}");

  static const  std::regex field_regex("[a-zA-Z][a-zA-Z0-9_]*");

  static const std::regex array_regex("(.+)(\\[([0-9]*)\\])");

  using std::regex;
  std::string::const_iterator begin = definition.begin();
  std::string::const_iterator end   = definition.end();
  std::match_results<std::string::const_iterator> what;

  // Get type and field
  std::string type, value;

  //-------------------------------
  // Find type, field and array size
  if( std::regex_search(begin, end, what, type_regex))
  {
    type = what[0];
    begin = what[0].second;
  }
  else {
    throw std::runtime_error("Bad type when parsing field: " + definition);
  }

  if (regex_search(begin, end, what, field_regex))
  {
    _fieldname = what[0];
    begin = what[0].second;
  }
  else {
    throw std::runtime_error("Bad field when parsing field: " + definition);
  }

  std::string temp_type = type;
  if (regex_search(temp_type, what, array_regex))
  {
    type = what[1];

    if (what.size() == 3) {
      _array_size = -1;
      _is_array = true;
    }
    else if (what.size() == 4) {
      std::string size(what[3].first, what[3].second);
      _array_size = size.empty() ? -1 : atoi(size.c_str());
      _is_array = true;
    }
    else {
      throw std::runtime_error("Bad array size when parsing field:  " + definition);
    }
  }

  //-------------------------------
  // Find if Constant or comment

  // Determine next character
  // if '=' -> constant, if '#' -> done, if nothing -> done, otherwise error
  if (regex_search(begin, end, what, std::regex("\\S")))
  {
    if (what[0] == "=")
    {
      begin = what[0].second;
      // Copy constant
      if (type == "string") {
        value.assign(begin, end);
      }
      else
      {
        if (regex_search(begin, end, what, std::regex("\\s*#")))
        {
          value.assign(begin, what[0].first);
        }
        else {
          value.assign(begin, end);
        }
      }

      TrimString(value);
      _is_constant = true;
    }
    else if (what[0] == "#")
    {
      // Ignore comment
    }
    else // default value, not constant ?
    {
      if (regex_search(begin, end, what, std::regex("\\s*#")))
      {
        value.assign(begin, what[0].first);
      }
      else {
        value.assign(begin, end);
      }
    }
  }
  _type  = ROSType( type );
  // TODO: Raise error if string is not numeric ?
  _value = value;
}

std::shared_ptr<ROSMessage> ROSField::getMessagePtr(const RosMessageLibrary &library) const
{
  if( _type.typeID() != BuiltinType::OTHER )
  {
    return {};
  }
  if( &library == _cache_library && _cache_message )
  {
    return _cache_message;
  }
  auto it = library.find(_type.baseName());
  if( it == library.end() )
  {
    return nullptr;
  }
  _cache_library = &library;
  _cache_message = it->second;
  return _cache_message;
}

void TrimStringLeft(std::string &s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
}

void TrimStringRight(std::string &s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

void TrimString(std::string &s)
{
  TrimStringLeft(s);
  TrimStringRight(s);
}

}
