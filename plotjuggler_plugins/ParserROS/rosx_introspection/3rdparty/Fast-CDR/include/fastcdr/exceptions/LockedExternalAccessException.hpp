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

#ifndef _FASTCDR_EXCEPTIONS_LOCKEDEXTERNALACCESSEXCEPTION_H_
#define _FASTCDR_EXCEPTIONS_LOCKEDEXTERNALACCESSEXCEPTION_H_

#include "Exception.h"

namespace eprosima {
namespace fastcdr {
namespace exception {
/*!
 * @brief This class is thrown as an exception when accessing to set the value of a locked external.
 * @ingroup EXCEPTIONMODULE
 */
class LockedExternalAccessException : public Exception
{
public:

    /*!
     * @brief Default constructor.
     *
     * @param message An error message. This message pointer is copied.
     */
    Cdr_DllAPI LockedExternalAccessException(
            const char* const& message) noexcept;

    /*!
     * @brief Default copy constructor.
     *
     * @param ex LockedExternalAccessException that will be copied.
     */
    Cdr_DllAPI LockedExternalAccessException(
            const LockedExternalAccessException& ex) noexcept;

    /*!
     * @brief Default move constructor.
     *
     * @param ex LockedExternalAccessException that will be moved.
     */
    Cdr_DllAPI LockedExternalAccessException(
            LockedExternalAccessException&& ex) noexcept;

    /*!
     * @brief Assigment operation.
     *
     * @param ex LockedExternalAccessException that will be copied.
     */
    Cdr_DllAPI LockedExternalAccessException& operator =(
            const LockedExternalAccessException& ex) noexcept;

    /*!
     * @brief Assigment operation.
     *
     * @param ex LockedExternalAccessException that will be moved.
     */
    LockedExternalAccessException& operator =(
            LockedExternalAccessException&& ex) noexcept;

    //! @brief Default destructor
    virtual Cdr_DllAPI ~LockedExternalAccessException() noexcept;

    //! @brief This function throws the object as exception.
    Cdr_DllAPI void raise() const override;

    //! @brief Default message used in the library.
    static Cdr_DllAPI const char* const LOCKED_EXTERNAL_ACCESS_MESSAGE_DEFAULT;
};
}         //namespace exception
}     //namespace fastcdr
} //namespace eprosima
#endif // _FASTCDR_EXCEPTIONS_LOCKEDEXTERNALACCESSEXCEPTION_H_
