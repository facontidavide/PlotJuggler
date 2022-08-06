/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright 2016-2017 Davide Faconti
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
* *******************************************************************/

#ifndef VARIANT_H
#define VARIANT_H

#include <string.h>
#include <type_traits>
#include <limits>
#include <string_view>
#include "rosx_introspection/builtin_types.hpp"
#include "rosx_introspection/details/exceptions.hpp"
#include "rosx_introspection/details/conversion_impl.hpp"

namespace RosMsgParser
{

class Variant
{

public:

  Variant() {
    _type = OTHER;
    _storage.raw_string = nullptr;
  }

  ~Variant();

  Variant(const Variant& other): _type(OTHER)
  {
    if( other._type == STRING)
    {
      const char* raw = other._storage.raw_string;
      const uint32_t size = *(reinterpret_cast<const uint32_t*>( &raw[0] ));
      const char* data = (&raw[4]);
      assign( data, size );
    }
    else{
      _type = other._type;
      _storage.raw_data = other._storage.raw_data;
    }
  }

  Variant(Variant&& other): _type(OTHER)
  {
    if( other._type == STRING)
    {
      _storage.raw_string = nullptr;
      std::swap( _storage.raw_string,  other._storage.raw_string);
      std::swap( _type, other._type);
    }
    else{
      _type = other._type;
      _storage.raw_data = other._storage.raw_data;
    }
  }

  Variant& operator = (const Variant& other)
  {
    if( other._type == STRING)
    {
      const char* raw = other._storage.raw_string;
      const uint32_t size = *(reinterpret_cast<const uint32_t*>( &raw[0] ));
      const char* data = (&raw[4]);
      assign( data, size );
    }
    else{
      _type = other._type;
      _storage.raw_data = other._storage.raw_data;
    }
    return *this;
  }

  template<typename T> Variant(const T& value);

  // specialization for raw string
  Variant(const char* buffer, size_t length);

  BuiltinType getTypeID() const;

  template<typename T> T convert( ) const;

  template<typename T> T extract( ) const;

  template <typename T> void assign(const T& value);

  void assign(const char* buffer, size_t length);

  // Direct access to raw data. Undefined behavior if this variant holds a STRING
  const uint8_t* getRawStorage() const;

private:

  union {
    std::array<uint8_t,8> raw_data;
    char* raw_string;
  }_storage;

  void clearStringIfNecessary();

  BuiltinType _type;
};


//----------------------- Implementation ----------------------------------------------


template<typename T>
inline Variant::Variant(const T& value):
  _type(OTHER)
{
  static_assert (std::numeric_limits<T>::is_specialized ||
                 std::is_same<T, RosMsgParser::Time>::value ||
                 std::is_same<T, std::string_view>::value ||
                 std::is_same<T, std::string>::value
                 , "not a valid type");

  _storage.raw_string = (nullptr);
  assign(value);
}

inline Variant::Variant(const char* buffer, size_t length):_type(OTHER)
{
  _storage.raw_string = (nullptr);
  assign(buffer,length);
}

inline Variant::~Variant()
{
  clearStringIfNecessary();
}

//-------------------------------------

inline BuiltinType Variant::getTypeID() const {
  return _type;
}

inline const uint8_t* Variant::getRawStorage() const { 
  return _storage.raw_data.data(); 
}

template<typename T> inline T Variant::extract( ) const
{
  static_assert (std::numeric_limits<T>::is_specialized ||
                 std::is_same<T, RosMsgParser::Time>::value
                 , "not a valid type");

  if( _type != RosMsgParser::getType<T>() )
  {
    throw TypeException("Variant::extract -> wrong type");
  }
  return * reinterpret_cast<const T*>( &_storage.raw_data[0] );
}

template<> inline std::string_view Variant::extract( ) const
{

  if( _type != STRING )
  {
    throw TypeException("Variant::extract -> wrong type");
  }
  const uint32_t size = *(reinterpret_cast<const uint32_t*>( &_storage.raw_string[0] ));
  char* data = static_cast<char*>(&_storage.raw_string[4]);
  return std::string_view(data, size);
}

template<> inline std::string Variant::extract( ) const
{
  if( _type != STRING )
  {
    throw TypeException("Variant::extract -> wrong type");
  }
  const uint32_t size = *(reinterpret_cast<const uint32_t*>( &_storage.raw_string[0] ));
  char* data = static_cast<char*>(&_storage.raw_string[4]);
  return std::string(data, size);
}

//-------------------------------------

template <typename T> inline void Variant::assign(const T& value)
{
  static_assert (std::numeric_limits<T>::is_specialized ||
                 std::is_same<T, RosMsgParser::Time>::value
                 , "not a valid type");

  clearStringIfNecessary();
  _type = RosMsgParser::getType<T>() ;
  *reinterpret_cast<T *>( &_storage.raw_data[0] ) =  value;
}

inline void Variant::clearStringIfNecessary()
{
  if( _storage.raw_string && _type == STRING)
  {
    delete [] _storage.raw_string;
    _storage.raw_string = nullptr;
  }
}

inline void Variant::assign(const char* buffer, size_t size)
{
  clearStringIfNecessary();
  _type = STRING;

  _storage.raw_string = new char[size+5];
  *reinterpret_cast<uint32_t *>( &_storage.raw_string[0] ) = size;
  memcpy(&_storage.raw_string[4] , buffer, size );
  _storage.raw_string[size+4] = '\0';
}



template <> inline void Variant::assign(const std::string_view& value)
{
  assign( value.data(), value.size() );
}

template <> inline void Variant::assign(const std::string& value)
{
  assign( value.data(), value.size() );
}

//-------------------------------------

template<typename DST> inline DST Variant::convert() const
{
  static_assert (std::numeric_limits<DST>::is_specialized ||
                 std::is_same<DST, RosMsgParser::Time>::value
                 , "not a valid type");

  using namespace RosMsgParser::details;
  DST target;

  const auto& raw_data = &_storage.raw_data[0];
  //----------
  switch( _type )
  {
  case CHAR:
  case INT8:   convert_impl<int8_t,  DST>(*reinterpret_cast<const int8_t*>( raw_data), target  ); break;

  case INT16:  convert_impl<int16_t, DST>(*reinterpret_cast<const int16_t*>( raw_data), target  ); break;
  case INT32:  convert_impl<int32_t, DST>(*reinterpret_cast<const int32_t*>( raw_data), target  ); break;
  case INT64:  convert_impl<int64_t, DST>(*reinterpret_cast<const int64_t*>( raw_data), target  ); break;

  case BOOL:
  case BYTE:
  case UINT8:   convert_impl<uint8_t,  DST>(*reinterpret_cast<const uint8_t*>( raw_data), target  ); break;

  case UINT16:  convert_impl<uint16_t, DST>(*reinterpret_cast<const uint16_t*>( raw_data), target  ); break;
  case UINT32:  convert_impl<uint32_t, DST>(*reinterpret_cast<const uint32_t*>( raw_data), target  ); break;
  case UINT64:  convert_impl<uint64_t, DST>(*reinterpret_cast<const uint64_t*>( raw_data), target  ); break;

  case FLOAT32:  convert_impl<float, DST>(*reinterpret_cast<const float*>( raw_data), target  ); break;
  case FLOAT64:  convert_impl<double, DST>(*reinterpret_cast<const double*>( raw_data), target  ); break;

  case STRING: {
    throw TypeException("String will not be converted to a numerical value implicitly");
 } break;

  case DURATION:
  case TIME: {
     throw TypeException("ros::Duration and ros::Time can be converted only to double (will be seconds)");
  } break;

  default: throw TypeException("Variant::convert -> cannot convert type" + std::to_string(_type)); break;

  }
  return  target;
}

template<> inline double Variant::convert() const
{
  using namespace RosMsgParser::details;
  double target = 0;
  const auto& raw_data = &_storage.raw_data[0];
  //----------
  switch( _type )
  {
  case CHAR:
  case INT8:   convert_impl<int8_t,  double>(*reinterpret_cast<const int8_t*>( raw_data), target  ); break;

  case INT16:  convert_impl<int16_t, double>(*reinterpret_cast<const int16_t*>( raw_data), target  ); break;
  case INT32:  convert_impl<int32_t, double>(*reinterpret_cast<const int32_t*>( raw_data), target  ); break;
  case INT64:  convert_impl<int64_t, double>(*reinterpret_cast<const int64_t*>( raw_data), target  ); break;

  case BOOL:
  case BYTE:
  case UINT8:   convert_impl<uint8_t,  double>(*reinterpret_cast<const uint8_t*>( raw_data), target  ); break;

  case UINT16:  convert_impl<uint16_t, double>(*reinterpret_cast<const uint16_t*>( raw_data), target  ); break;
  case UINT32:  convert_impl<uint32_t, double>(*reinterpret_cast<const uint32_t*>( raw_data), target  ); break;
  case UINT64:  convert_impl<uint64_t, double>(*reinterpret_cast<const uint64_t*>( raw_data), target  ); break;

  case FLOAT32:  convert_impl<float, double>(*reinterpret_cast<const float*>( raw_data), target  ); break;
  case FLOAT64:  return extract<double>();

  case STRING: {
    throw TypeException("String will not be converted to a double implicitly");
  }break;

  case DURATION:
  case TIME: {
    RosMsgParser::Time tmp =  extract<RosMsgParser::Time>();
    target = tmp.toSec();
  }break;

  default: throw TypeException("Variant::convert -> cannot convert type" + std::to_string(_type));

  }
  return  target;
}

template<> inline RosMsgParser::Time Variant::convert() const
{
  if(  _type != TIME )
  {
     throw TypeException("Variant::convert -> cannot convert RosMsgParser::Time");
  }
  return extract<RosMsgParser::Time>();
}


template<> inline std::string Variant::convert() const
{
  if(  _type != STRING )
  {
     throw TypeException("Variant::convert -> cannot convert to std::string");
  }
  return extract<std::string>();
}

} //end namespace


#endif // VARIANT_H
