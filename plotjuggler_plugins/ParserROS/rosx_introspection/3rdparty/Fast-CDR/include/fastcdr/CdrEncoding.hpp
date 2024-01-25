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

#ifndef _FASTCDR_CDRENCODING_HPP_
#define _FASTCDR_CDRENCODING_HPP_

#include <cstdint>

namespace eprosima {
namespace fastcdr {

//! @brief This enumeration represents the kinds of CDR serialization supported by eprosima::fastcdr::CDR.
typedef enum
{
    //! @brief Common CORBA CDR serialization.
    CORBA_CDR = 0,
    //! @brief DDS CDR serialization.
    DDS_CDR = 1,
    //! @brief XCDRv1 encoding defined by standard DDS X-Types 1.3
    XCDRv1 = 2,
    //! @brief XCDRv2 encoding defined by standard DDS X-Types 1.3
    XCDRv2 = 3
} CdrVersion;

//! @brief This enumeration represents the supported XCDR encoding algorithms.
typedef enum : uint8_t
{
    //! @brief Specifies that the content is PLAIN_CDR.
    PLAIN_CDR = 0x0,
    //! @brief Specifies that the content is PL_CDR,
    PL_CDR = 0x2,
    //! @brief Specifies that the content is PLAIN_CDR2.
    PLAIN_CDR2 = 0x6,
    //! @brief Specifies that the content is DELIMIT_CDR2.
    DELIMIT_CDR2 = 0x8,
    //! @brief Specifies that the content is PL_CDR2.
    PL_CDR2 = 0xa
} EncodingAlgorithmFlag;

} // namespace fastcdr
} // namespace eprosima

#endif // _FASTCDR_CDRENCODING_HPP_
