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

#include <fastcdr/exceptions/NotEnoughMemoryException.h>

using namespace eprosima::fastcdr::exception;

const char* const NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT =
        "Not enough memory in the buffer stream";

NotEnoughMemoryException::NotEnoughMemoryException(
        const char* const& message) noexcept
    : Exception(message)
{
}

NotEnoughMemoryException::NotEnoughMemoryException(
        const NotEnoughMemoryException& ex) noexcept
    : Exception(ex)
{
}

NotEnoughMemoryException::NotEnoughMemoryException(
        NotEnoughMemoryException&& ex) noexcept
    : Exception(std::move(ex))
{
}

NotEnoughMemoryException& NotEnoughMemoryException::operator =(
        const NotEnoughMemoryException& ex) noexcept
{
    if (this != &ex)
    {
        Exception::operator =(
                ex);
    }

    return *this;
}

NotEnoughMemoryException& NotEnoughMemoryException::operator =(
        NotEnoughMemoryException&& ex) noexcept
{
    if (this != &ex)
    {
        Exception::operator =(
                std::move(ex));
    }

    return *this;
}

NotEnoughMemoryException::~NotEnoughMemoryException() noexcept
{
}

void NotEnoughMemoryException::raise() const
{
    throw *this;
}
