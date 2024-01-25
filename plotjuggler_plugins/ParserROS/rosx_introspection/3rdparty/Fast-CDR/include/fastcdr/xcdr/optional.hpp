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
#ifndef _FASTCDR_XCDR_OPTIONAL_HPP_
#define _FASTCDR_XCDR_OPTIONAL_HPP_

#include <new>
#include <utility>

#include "detail/optional.hpp"
#include "../exceptions/BadOptionalAccessException.hpp"

namespace eprosima {
namespace fastcdr {

//! An empty class type used to indicate optional type with uninitialized state.
struct nullopt_t
{
    constexpr explicit nullopt_t(
            int)
    {
    }

};

/*!
 * @brief nullopt is a constant of type nullopt_t that is used to indicate optional type with uninitialized state.
 */
static constexpr nullopt_t nullopt {0};

/*!
 * @brief This class template manages an optional contained value, i.e. a value that may or may not be present.
 */
template<class T>
class optional
{
public:

    using type = T;

    //! Default constructor
    optional() = default;

    //! Copy constructor from an instance of the templated class.
    optional(
            const T& val) noexcept
    {
        ::new(&storage_.val_)T(val);
        storage_.engaged_ = true;
    }

    //! Move constructor from an instance of the templated class.
    optional(
            T&& val) noexcept
    {
        ::new(&storage_.val_)T(std::move(val));
        storage_.engaged_ = true;
    }

    //! Copy constructor.
    optional(
            const optional<T>& val) noexcept
    {
        ::new(&storage_.val_)T(val.storage_.val_);
        storage_.engaged_ = val.storage_.engaged_;
    }

    //! Move constructor.
    optional(
            optional<T>&& val) noexcept
    {
        ::new(&storage_.val_)T(std::move(val.storage_.val_));
        storage_.engaged_ = val.storage_.engaged_;
    }

    //! Destructor
    ~optional() = default;

    /*!
     * @brief Constructs the contained value in-place
     *
     * @param[in] _args The arguments to pass to the constructor.
     */
    template<class ... Args> void emplace(
            Args&&... _args)
    {
        reset();
        storage_.val_.T(std::forward<Args>(_args)...);
        storage_.engaged_ = true;
    }

    /*!
     * @brief Reset the state of the optional
     *
     * @param[in] initial_engaged True value initializes the state with a default instance of the templated class.
     * False value leaves the optional in a uninitialized state.
     */
    void reset(
            bool initial_engaged = false)
    {
        if (storage_.engaged_)
        {
            storage_.val_.~T();
        }
        storage_.engaged_ = initial_engaged;
        if (storage_.engaged_)
        {
            ::new(&storage_.val_)T();
        }
    }

    /*!
     * @brief Returns the contained value.
     *
     * @return The contained value.
     * @exception exception::BadOptionalAccessException This exception is thrown when the optional is uninitialized.
     */
    T& value()&
    {
        if (!storage_.engaged_)
        {
            throw exception::BadOptionalAccessException(
                      exception::BadOptionalAccessException::BAD_OPTIONAL_ACCESS_MESSAGE_DEFAULT);
        }

        return storage_.val_;
    }

    /*!
     * @brief Returns the contained value.
     *
     * @return The contained value.
     * @exception exception::BadOptionalAccessException This exception is thrown when the optional is uninitialized.
     */
    const T& value() const&
    {
        if (!storage_.engaged_)
        {
            throw exception::BadOptionalAccessException(
                      exception::BadOptionalAccessException::BAD_OPTIONAL_ACCESS_MESSAGE_DEFAULT);
        }

        return storage_.val_;
    }

    /*!
     * @brief Returns the contained value.
     *
     * @return The contained value.
     * @exception exception::BadOptionalAccessException This exception is thrown when the optional is uninitialized.
     */
    T&& value() &&
    {
        if (!storage_.engaged_)
        {
            throw exception::BadOptionalAccessException(
                      exception::BadOptionalAccessException::BAD_OPTIONAL_ACCESS_MESSAGE_DEFAULT);
        }

        return std::move(storage_.val_);
    }

    /*!
     * @brief Returns the contained value.
     *
     * @return The contained value.
     * @exception exception::BadOptionalAccessException This exception is thrown when the optional is uninitialized.
     */
    const T&& value() const&&
    {
        if (!storage_.engaged_)
        {
            throw exception::BadOptionalAccessException(
                      exception::BadOptionalAccessException::BAD_OPTIONAL_ACCESS_MESSAGE_DEFAULT);
        }

        return std::move(storage_.val_);
    }

    /*!
     * @brief Checks whether the optional contains a value.
     *
     * @return Whether the optional contains a value.
     */
    bool has_value() const
    {
        return storage_.engaged_;
    }

    //! Assigns content from an optional.
    optional& operator =(
            const optional& opt)
    {
        reset();
        storage_.engaged_ = opt.storage_.engaged_;
        if (opt.storage_.engaged_)
        {
            ::new(&storage_.val_)T(opt.storage_.val_);
        }
        return *this;
    }

    //! Assigns content from an optional.
    optional& operator =(
            optional&& opt)
    {
        reset();
        storage_.engaged_ = opt.storage_.engaged_;
        if (opt.storage_.engaged_)
        {
            ::new(&storage_.val_)T(std::move(opt.storage_.val_));
        }
        return *this;
    }

    //! Assigns content from an instance of the templated class.
    optional& operator =(
            const T& val)
    {
        reset();
        ::new(&storage_.val_)T(val);
        storage_.engaged_ = true;
        return *this;
    }

    //! Assigns content from an instance of the templated class.
    optional& operator =(
            T&& val)
    {
        reset();
        ::new(&storage_.val_)T(std::move(val));
        storage_.engaged_ = true;
        return *this;
    }

    //! Uninitialized the optional.
    optional& operator = (
            nullopt_t) noexcept
    {
        reset();
        return *this;
    }

    //! Compares optional values.
    bool operator ==(
            const optional& opt_val) const
    {
        return opt_val.storage_.engaged_ == storage_.engaged_ &&
               (storage_.engaged_ ? opt_val.storage_.val_ == storage_.val_ : true);
    }

    //! Compares optional values.
    bool operator !=(
            const optional& opt_val) const
    {
        return !operator ==(opt_val);
    }

    /*!
     * @brief Accesses the contained value.
     *
     * The behavior is undefined if *this does not contain a value.
     *
     * @return The contained value.
     */
    T& operator *() & noexcept
    {
        return storage_.val_;
    }

    /*!
     * @brief Accesses the contained value.
     *
     * The behavior is undefined if *this does not contain a value.
     *
     * @return The contained value.
     */
    const T& operator *() const& noexcept
    {
        return storage_.val_;
    }

    /*!
     * @brief Accesses the contained value.
     *
     * The behavior is undefined if *this does not contain a value.
     *
     * @return The contained value.
     */
    T&& operator *() && noexcept
    {
        return std::move(storage_.val_);
    }

    /*!
     * @brief Accesses the contained value.
     *
     * The behavior is undefined if *this does not contain a value.
     *
     * @return The contained value.
     */
    const T&& operator *() const&& noexcept
    {
        return std::move(storage_.val_);
    }

    /*!
     * @brief Accesses the contained value.
     *
     * The behavior is undefined if *this does not contain a value.
     *
     * @return The contained value.
     */
    T* operator ->() noexcept
    {
        return std::addressof(storage_.val_);
    }

    /*!
     * @brief Accesses the contained value.
     *
     * The behavior is undefined if *this does not contain a value.
     *
     * @return The contained value.
     */
    const T* operator ->() const noexcept
    {
        return std::addressof(storage_.val_);
    }

    //! Checks whether the optional contains a value.
    explicit operator bool() const noexcept
    {
        return storage_.engaged_;
    }

private:

    detail::optional_storage<T> storage_;
};

} // namespace fastcdr
} // namespace eprosima

#endif //_FASTCDR_XCDR_OPTIONAL_HPP_
