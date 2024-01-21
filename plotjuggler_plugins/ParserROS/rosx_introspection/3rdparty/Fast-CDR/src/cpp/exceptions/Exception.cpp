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

#include <fastcdr/exceptions/Exception.h>

using namespace eprosima::fastcdr::exception;

Exception::Exception(
        const char* const& message) noexcept
    : m_message(message)
{
}

Exception::Exception(
        const Exception& ex) noexcept
    : m_message(ex.m_message)
{
}

Exception::Exception(
        Exception&& ex) noexcept
    : m_message(std::move(ex.m_message))
{
}

Exception& Exception::operator =(
        const Exception& ex) noexcept
{
    m_message = ex.m_message;
    return *this;
}

Exception& Exception::operator =(
        Exception&& ex) noexcept
{
    m_message = std::move(ex.m_message);
    return *this;
}

Exception::~Exception() noexcept
{
}

const char* Exception::what() const noexcept
{
    return m_message;
}
