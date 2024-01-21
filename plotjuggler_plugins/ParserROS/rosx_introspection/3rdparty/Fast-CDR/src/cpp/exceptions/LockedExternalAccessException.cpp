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

#include <fastcdr/exceptions/LockedExternalAccessException.hpp>

using namespace eprosima::fastcdr::exception;

const char* const LockedExternalAccessException::LOCKED_EXTERNAL_ACCESS_MESSAGE_DEFAULT =
        "Locked external: value cannot be set";

LockedExternalAccessException::LockedExternalAccessException(
        const char* const& message) noexcept
    : Exception(message)
{
}

LockedExternalAccessException::LockedExternalAccessException(
        const LockedExternalAccessException& ex) noexcept
    : Exception(ex)
{
}

LockedExternalAccessException::LockedExternalAccessException(
        LockedExternalAccessException&& ex) noexcept
    : Exception(std::move(ex))
{
}

LockedExternalAccessException& LockedExternalAccessException::operator =(
        const LockedExternalAccessException& ex) noexcept
{
    if (this != &ex)
    {
        Exception::operator =(
                ex);
    }

    return *this;
}

LockedExternalAccessException& LockedExternalAccessException::operator =(
        LockedExternalAccessException&& ex) noexcept
{
    if (this != &ex)
    {
        Exception::operator =(
                std::move(ex));
    }

    return *this;
}

LockedExternalAccessException::~LockedExternalAccessException() noexcept
{
}

void LockedExternalAccessException::raise() const
{
    throw *this;
}
