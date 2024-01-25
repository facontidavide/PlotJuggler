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
//
#ifndef _FASTCDR_XCDR_DETAIL_OPTIONAL_HPP_
#define _FASTCDR_XCDR_DETAIL_OPTIONAL_HPP_

#include <type_traits>

namespace eprosima {
namespace fastcdr {
namespace detail {
template<class T, typename = void>
struct optional_storage
{
    union
    {
        char dummy_;
        T val_;
    };

    bool engaged_ { false };

    optional_storage()
    {
    }

    ~optional_storage()
    {
        if (engaged_)
        {
            val_.~T();
        }
    }

};

/* *INDENT-OFF* */
template<class T>
struct optional_storage<T, typename std::enable_if<std::is_trivially_destructible<T>{}>::type>
{
    union
    {
        char dummy_; T val_;
    };

    bool engaged_ { false };

    optional_storage()
    {
    }

    ~optional_storage() = default;
};
/* *INDENT-ON* */
} // namespace detail
} // namespace fastcdr
} // namespace eprosima

#endif //_FASTCDR_XCDR_DETAIL_OPTIONAL_HPP_

