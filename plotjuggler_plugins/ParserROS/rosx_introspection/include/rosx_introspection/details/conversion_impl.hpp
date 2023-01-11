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

#ifndef VARIANT_IMPL_H
#define VARIANT_IMPL_H

#include <type_traits>
#include <limits>
#include <iostream>
#include <cmath>
#include "rosx_introspection/builtin_types.hpp"
#include "rosx_introspection/details/exceptions.hpp"

namespace RosMsgParser
{

namespace details{

template <typename T>
using Invoke = typename T::type;


template <typename BoolCondition>
using EnableIf = Invoke<std::enable_if<BoolCondition::value> >;


template <typename T>
struct is_integer : std::integral_constant<bool, std::is_integral<T>::value
                                                   && !std::is_same<T, bool>::value
                                                   && !std::is_same<T, char>::value>
{};

template <typename From, typename To>
struct is_same_real : std::integral_constant<bool,
                                             std::is_same<From, To>::value
                                               && std::is_floating_point<To>::value >
{};


template <typename From, typename To>
struct is_safe_integer_conversion
  : std::integral_constant<bool, is_integer<From>::value
                                   && is_integer<To>::value
                                   && sizeof(From) <= sizeof(To)
                                   && std::is_signed<From>::value == std::is_signed<To>::value>
{};

template <typename From, typename To>
struct float_conversion
  : std::integral_constant<bool, std::is_floating_point<From>::value
                                   && std::is_floating_point<To>::value
        && !std::is_same<From,To>::value >
{};

template <typename From, typename To>
struct unsigned_to_smaller_conversion
  : std::integral_constant<bool, is_integer<From>::value
                                   && is_integer<To>::value
                                   && (sizeof(From) > sizeof(To))
                                   && !std::is_signed<From>::value
                                   && !std::is_signed<To>::value >
{};

template <typename From, typename To>
struct signed_to_smaller_conversion
  : std::integral_constant<bool, is_integer<From>::value
                                   && is_integer<To>::value
                                   && (sizeof(From) > sizeof(To))
                                   && std::is_signed<From>::value
                                   && std::is_signed<To>::value >
{};

//---------------------------
template <typename From, typename To>
struct signed_to_smaller_unsigned_conversion
  : std::integral_constant<bool, is_integer<From>::value
                                   && is_integer<To>::value
                                   && sizeof(From) >= sizeof(To)
                                   && std::is_signed<From>::value
                                   && !std::is_signed<To>::value >
{};

template <typename From, typename To>
struct signed_to_larger_unsigned_conversion
  : std::integral_constant<bool, is_integer<From>::value
                                   && is_integer<To>::value
                                   && sizeof(From) < sizeof(To)
                                   && std::is_signed<From>::value
                                   && !std::is_signed<To>::value >
{};

template <typename From, typename To>
struct unsigned_to_smaller_signed_conversion
  : std::integral_constant<bool, is_integer<From>::value
                                   && is_integer<To>::value
                                   && (sizeof(From) >= sizeof(To))
                                   && !std::is_signed<From>::value
                                   && std::is_signed<To>::value >
{};

template <typename From, typename To>
struct unsigned_to_larger_signed_conversion
  : std::integral_constant<bool, is_integer<From>::value
                                   && is_integer<To>::value
                                   && sizeof(From) < sizeof(To)
                                   && !std::is_signed<From>::value
                                   && std::is_signed<To>::value >
{};

template <typename From, typename To>
struct floating_to_signed_conversion
  : std::integral_constant<bool, std::is_floating_point<From>::value
                                   && is_integer<To>::value
                                   && std::is_signed<To>::value >
{};

template <typename From, typename To>
struct floating_to_unsigned_conversion
  : std::integral_constant<bool, std::is_floating_point<From>::value
                                   && is_integer<To>::value
                                   && !std::is_signed<To>::value >
{};

template <typename From, typename To>
struct integer_to_floating_conversion
  : std::integral_constant<bool, is_integer<From>::value
                                   && std::is_floating_point<To>::value >
{};

template <typename From, typename To>
inline void checkUpperLimit(const From& from)
{
  if ((sizeof(To) < sizeof(From)) &&
      (from > static_cast<From>(std::numeric_limits<To>::max()))) {
    throw RangeException("Value too large.");
  }
  else if (static_cast<To>(from) > std::numeric_limits<To>::max()) {
    throw RangeException("Value too large.");
  }
}

template <typename From, typename To>
inline void checkUpperLimitFloat(const From& from)
{
  if (from > std::numeric_limits<To>::max()){
    throw RangeException("Value too large.");
  }
}

template <typename From, typename To>
inline void checkLowerLimitFloat(const From& from)
{
  if (from < -std::numeric_limits<To>::max()){
    throw RangeException("Value too small.");
  }
}

template <typename From, typename To>
inline void checkLowerLimit(const From& from)
{
  if (from < std::numeric_limits<To>::min()){
    throw RangeException("Value too small.");
  }
}

template <typename From, typename To>
inline void checkTruncation(const From& from)
{
  if(from != static_cast<From>(static_cast<To>( from))){
    throw RangeException("Floating point truncated");
  }
}


//----------------------- Implementation ----------------------------------------------

template<typename SRC,typename DST,
          typename details::EnableIf< details::is_same_real<SRC, DST>>* = nullptr >
inline void convert_impl( const SRC& from, DST& target )
{
  target = from;
}


template<typename SRC,typename DST,
          typename details::EnableIf< details::is_safe_integer_conversion<SRC, DST>>* = nullptr >
inline void convert_impl( const SRC& from, DST& target )
{
  target = static_cast<DST>( from);
}

template<typename SRC,typename DST,
          typename details::EnableIf< details::float_conversion<SRC, DST>>* = nullptr >
inline void convert_impl( const SRC& from, DST& target )
{
  if(!std::isnan(from)) {
    checkTruncation<SRC,DST>(from);
  }
  target = static_cast<DST>( from );
}


template<typename SRC,typename DST,
          typename details::EnableIf< details::unsigned_to_smaller_conversion<SRC, DST>>* = nullptr  >
inline void convert_impl( const SRC& from, DST& target )
{
  checkUpperLimit<SRC,DST>(from);
  target = static_cast<DST>(from);
}

template<typename SRC,typename DST,
          typename details::EnableIf< details::signed_to_smaller_conversion<SRC, DST>>* = nullptr  >
inline void convert_impl( const SRC& from, DST& target )
{
  checkLowerLimit<SRC,DST>(from);
  checkUpperLimit<SRC,DST>(from);
  target = static_cast<DST>( from);
}


template<typename SRC,typename DST,
          typename details::EnableIf< details::signed_to_smaller_unsigned_conversion<SRC, DST>>* = nullptr  >
inline void convert_impl( const SRC& from, DST& target )
{
  if (from < 0){
    throw RangeException("Value is negative and can't be converted to signed");
  }

  checkUpperLimit<SRC,DST>(from);
  target = static_cast<DST>( from);
}


template<typename SRC,typename DST,
          typename details::EnableIf< details::signed_to_larger_unsigned_conversion<SRC, DST>>* = nullptr   >
inline void convert_impl( const SRC& from, DST& target )
{
  if (from < 0){
    throw RangeException("Value is negative and can't be converted to signed");
  }

  target = static_cast<DST>( from);
}

template<typename SRC,typename DST,
          typename details::EnableIf< details::unsigned_to_larger_signed_conversion<SRC, DST>>* = nullptr   >
inline void convert_impl( const SRC& from, DST& target )
{
  target = static_cast<DST>( from);
}

template<typename SRC,typename DST,
          typename details::EnableIf< details::unsigned_to_smaller_signed_conversion<SRC, DST>>* = nullptr   >
inline void convert_impl( const SRC& from, DST& target )
{
  checkUpperLimit<SRC,DST>(from);
  target = static_cast<DST>( from);
}

template<typename SRC,typename DST,
          typename details::EnableIf< details::floating_to_signed_conversion<SRC, DST>>* = nullptr   >
inline void convert_impl( const SRC& from, DST& target )
{
  checkLowerLimitFloat<SRC,DST>(from);
  checkUpperLimitFloat<SRC,DST>(from);

  if( from != static_cast<SRC>(static_cast<DST>( from))){
      throw RangeException("Floating point truncated");
  }

  target = static_cast<DST>( from);
}

template<typename SRC,typename DST,
          typename details::EnableIf< details::floating_to_unsigned_conversion<SRC, DST>>* = nullptr   >
inline void convert_impl( const SRC& from, DST& target )
{
  if (from < 0 ){
    throw RangeException("Value is negative and can't be converted to signed");
  }

  checkLowerLimitFloat<SRC,DST>(from);

  if( from != static_cast<SRC>(static_cast<DST>( from))){
    throw RangeException("Floating point truncated");
  }

  target = static_cast<DST>( from);
}

template<typename SRC,typename DST,
          typename details::EnableIf< details::integer_to_floating_conversion<SRC, DST>>* = nullptr >
inline void convert_impl( const SRC& from, DST& target )
{
  checkTruncation<SRC,DST>(from);
  target = static_cast<DST>( from);
}

} //end namespace details

} //end namespace

#endif // VARIANT_H

