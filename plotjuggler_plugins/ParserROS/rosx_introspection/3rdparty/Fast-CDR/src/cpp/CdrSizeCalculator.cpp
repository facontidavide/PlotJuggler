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

#include <fastcdr/CdrSizeCalculator.hpp>

namespace eprosima {
namespace fastcdr {

CdrSizeCalculator::CdrSizeCalculator(
        CdrVersion cdr_version)
    : cdr_version_(cdr_version)
{
    switch (cdr_version_)
    {
        case CdrVersion::XCDRv2:
            break;
        default:
            current_encoding_ = EncodingAlgorithmFlag::PLAIN_CDR;
            align64_ = 8;
            break;
    }
}

CdrVersion CdrSizeCalculator::get_cdr_version() const
{
    return cdr_version_;
}

EncodingAlgorithmFlag CdrSizeCalculator::get_encoding() const
{
    return current_encoding_;
}

size_t CdrSizeCalculator::begin_calculate_type_serialized_size(
        EncodingAlgorithmFlag new_encoding,
        size_t& current_alignment)
{
    size_t calculated_size {0};
    current_encoding_ = new_encoding;

    if (CdrVersion::XCDRv2 == cdr_version_ && EncodingAlgorithmFlag::PLAIN_CDR2 != current_encoding_)
    {
        calculated_size = 4 + alignment(current_alignment, 4);     // DHEADER
    }

    current_alignment += calculated_size;

    serialized_member_size_ = NO_SERIALIZED_MEMBER_SIZE;     // Avoid error when serializing arrays, sequences, etc..

    return calculated_size;
}

size_t CdrSizeCalculator::end_calculate_type_serialized_size(
        EncodingAlgorithmFlag new_encoding,
        size_t& current_alignment)
{
    size_t calculated_size {0};

    if (CdrVersion::XCDRv1 == cdr_version_ && EncodingAlgorithmFlag::PL_CDR == current_encoding_)
    {
        calculated_size = 4 + alignment(current_alignment, 4);     // Sentinel
    }
    else if (CdrVersion::XCDRv2 == cdr_version_ && EncodingAlgorithmFlag::PLAIN_CDR2 != current_encoding_)
    {
        serialized_member_size_ = SERIALIZED_MEMBER_SIZE;
    }

    current_alignment += calculated_size;

    current_encoding_ = new_encoding;

    return calculated_size;
}

} // namespace fastcdr
} // namespace eprosima
