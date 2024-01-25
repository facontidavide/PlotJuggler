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
#ifndef _FASTCDR_XCDR_EXTERNAL_HPP_
#define _FASTCDR_XCDR_EXTERNAL_HPP_

#include <memory>

#include "../exceptions/LockedExternalAccessException.hpp"

namespace eprosima {
namespace fastcdr {

/*!
 * @brief This class template manages an external member, a member declared to be external to the storage of a type.
 */
template<class T>
class external
{
public:

    using type = T;

    //! Default constructor
    external() = default;

    //! Constructor from a pointer.
    external(
            T* pointer,
            bool locked = false) noexcept
        : pointer_ {pointer}
        , locked_ {locked}
    {
    }

    //! Constructor from a shared pointer.
    external(
            std::shared_ptr<T> pointer) noexcept
        : pointer_ {pointer}
    {
    }

    //! Copy constructor.
    external(
            const external<T>& other) noexcept
        : locked_ {other.locked_}
    {
        if (locked_)
        {
            pointer_ = std::make_shared<T>(*other.pointer_);
        }
        else
        {
            pointer_ = other.pointer_;
        }
    }

    //! Assignment
    external<T>& operator =(
            const external<T>& other)
    {
        if (locked_)
        {
            throw exception::LockedExternalAccessException(
                      exception::LockedExternalAccessException::LOCKED_EXTERNAL_ACCESS_MESSAGE_DEFAULT);
        }

        if (!other.pointer_)
        {
            pointer_.reset();
        }
        else if (other.locked_)
        {
            if (!pointer_)
            {
                pointer_ = std::make_shared<T>(*other.pointer_);
            }
            else
            {
                *pointer_ = *other.pointer_;
            }
        }
        else
        {
            pointer_ = other.pointer_;
        }

        return *this;
    }

    //! Destructor
    ~external() = default;

    //! Dereference object.
    T& operator *() noexcept
    {
        return *pointer_;
    }

    //! Dereference object.
    const T& operator *() const noexcept
    {
        return *pointer_;
    }

    //! Get pointer.
    T* get() noexcept
    {
        return pointer_.get();
    }

    //! Get pointer.
    const T* get() const noexcept
    {
        return pointer_.get();
    }

    //! Get shared pointer.
    std::shared_ptr<T> get_shared_ptr() noexcept
    {
        return pointer_;
    }

    //! Dereference object member.
    T* operator ->() noexcept
    {
        return pointer_.get();
    }

    //! Dereference object member.
    const T* operator ->() const noexcept
    {
        return pointer_.get();
    }

    //! Compares they manage the same object or empty both.
    bool operator ==(
            const external<T>& other) const
    {
        return pointer_.get() == other.pointer_.get();
    }

    //! Compares they don't manages the same object
    bool operator !=(
            const external<T>& other) const
    {
        return !(*this == other);
    }

    //! Checks if not null
    operator bool() const noexcept
    {
        return nullptr != pointer_.get();
    }

    //! Checks if locked
    bool is_locked() const noexcept
    {
        return locked_;
    }

    //! Locks the managed object.
    void lock() noexcept
    {
        locked_ = true;
    }

private:

    std::shared_ptr<T> pointer_;

    bool locked_ {false};

};

} // namespace fastcdr
} // namespace eprosima

#endif //_FASTCDR_XCDR_EXTERNAL_HPP_
