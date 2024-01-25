// Copyright 2023 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _FASTCDR_DETAIL_CONTAINERRECURSIVEINSPECTOR_HPP_
#define _FASTCDR_DETAIL_CONTAINERRECURSIVEINSPECTOR_HPP_

#include <array>
#include <cstddef>
#include <type_traits>

namespace eprosima {
namespace fastcdr {

// Helpers to deduce multi-array of primitives.
/// Basis
constexpr bool is_multi_array_primitive(
        ...)
{
    return false;
}

/// Specializations
template <typename _T,
        typename std::enable_if<std::is_enum<_T>::value ||
        std::is_arithmetic<_T>::value>::type* = nullptr>
constexpr bool is_multi_array_primitive(
        _T const*)
{
    return true;
}

template <typename _T, size_t _N>
constexpr bool is_multi_array_primitive(
        std::array<_T, _N> const*)
{
    return is_multi_array_primitive(static_cast<_T const*>(nullptr));
}

} // namespace fastcdr
} // namespace eprosima

#endif // _FASTCDR_DETAIL_CONTAINERRECURSIVEINSPECTOR_HPP_
