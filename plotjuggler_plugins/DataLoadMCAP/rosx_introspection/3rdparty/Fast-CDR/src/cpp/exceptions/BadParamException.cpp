// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <fastcdr/exceptions/BadParamException.h>

using namespace eprosima::fastcdr::exception;

const char* const BadParamException::BAD_PARAM_MESSAGE_DEFAULT = "Bad parameter";

BadParamException::BadParamException(
        const char* const& message) noexcept
    : Exception(message)
{
}

BadParamException::BadParamException(
        const BadParamException& ex) noexcept
    : Exception(ex)
{
}

#if HAVE_CXX0X
BadParamException::BadParamException(
        BadParamException&& ex) noexcept
    : Exception(std::move(ex))
{
}

#endif // if HAVE_CXX0X

BadParamException& BadParamException::operator =(
        const BadParamException& ex) noexcept
{
    if (this != &ex)
    {
        Exception::operator =(
                ex);
    }

    return *this;
}

#if HAVE_CXX0X
BadParamException& BadParamException::operator =(
        BadParamException&& ex) noexcept
{
    if (this != &ex)
    {
        Exception::operator =(
                std::move(ex));
    }

    return *this;
}

#endif // if HAVE_CXX0X

BadParamException::~BadParamException() noexcept
{
}

void BadParamException::raise() const
{
    throw *this;
}
