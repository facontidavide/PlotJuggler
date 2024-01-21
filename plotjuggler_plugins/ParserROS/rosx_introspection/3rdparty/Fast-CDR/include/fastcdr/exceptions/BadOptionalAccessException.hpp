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

#ifndef _FASTCDR_EXCEPTIONS_BADOPTIONALACCESSEXCEPTION_H_
#define _FASTCDR_EXCEPTIONS_BADOPTIONALACCESSEXCEPTION_H_

#include "Exception.h"

namespace eprosima {
namespace fastcdr {
namespace exception {
/*!
 * @brief This class is thrown as an exception when accessing the value of a null optional.
 * @ingroup EXCEPTIONMODULE
 */
class BadOptionalAccessException : public Exception
{
public:

    /*!
     * @brief Default constructor.
     *
     * @param message An error message. This message pointer is copied.
     */
    Cdr_DllAPI BadOptionalAccessException(
            const char* const& message) noexcept;

    /*!
     * @brief Default copy constructor.
     *
     * @param ex BadOptionalAccessException that will be copied.
     */
    Cdr_DllAPI BadOptionalAccessException(
            const BadOptionalAccessException& ex) noexcept;

    /*!
     * @brief Default move constructor.
     *
     * @param ex BadOptionalAccessException that will be moved.
     */
    Cdr_DllAPI BadOptionalAccessException(
            BadOptionalAccessException&& ex) noexcept;

    /*!
     * @brief Assigment operation.
     *
     * @param ex BadOptionalAccessException that will be copied.
     */
    Cdr_DllAPI BadOptionalAccessException& operator =(
            const BadOptionalAccessException& ex) noexcept;

    /*!
     * @brief Assigment operation.
     *
     * @param ex BadOptionalAccessException that will be moved.
     */
    BadOptionalAccessException& operator =(
            BadOptionalAccessException&& ex) noexcept;

    //! @brief Default destructor
    virtual Cdr_DllAPI ~BadOptionalAccessException() noexcept;

    //! @brief This function throws the object as exception.
    Cdr_DllAPI void raise() const override;

    //! @brief Default message used in the library.
    static Cdr_DllAPI const char* const BAD_OPTIONAL_ACCESS_MESSAGE_DEFAULT;
};
}         //namespace exception
}     //namespace fastcdr
} //namespace eprosima
#endif // _FASTCDR_EXCEPTIONS_BADOPTIONALACCESSEXCEPTION_H_
