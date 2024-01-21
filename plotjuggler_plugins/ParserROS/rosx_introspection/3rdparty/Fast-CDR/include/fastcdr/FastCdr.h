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

#ifndef _FASTCDR_FASTCDR_H_
#define _FASTCDR_FASTCDR_H_

#include "fastcdr_dll.h"
#include "FastBuffer.h"
#include "exceptions/NotEnoughMemoryException.h"
#include <stdint.h>
#include <string>
#include <vector>

#if !__APPLE__ && !__FreeBSD__ && !__VXWORKS__
#include <malloc.h>
#else
#include <stdlib.h>
#endif // if !__APPLE__ && !__FreeBSD__ && !__VXWORKS__

#include <array>

namespace eprosima {
namespace fastcdr {
/*!
 * @brief This class offers an interface to serialize/deserialize some basic types using a modified CDR protocol inside a eprosima::FastBuffer.
 * This modified CDR protocol provides a serialization mechanism much faster than common CDR protocol, because it doesn't use alignment.
 * @ingroup FASTCDRAPIREFERENCE
 */
class Cdr_DllAPI FastCdr
{
public:

    /*!
     * @brief This class stores the current state of a CDR serialization.
     */
    class Cdr_DllAPI state
    {
        friend class FastCdr;

    public:

        /*!
         * @brief Default constructor.
         */
        state(
                const FastCdr& fastcdr);

        /*!
         * @brief Copy constructor.
         */
        state(
                const state&);

    private:

        state& operator =(
                const state&) = delete;

        //! @brief The position in the buffer when the state was created.
        const FastBuffer::iterator current_position_;
    };
    /*!
     * @brief This constructor creates a eprosima::fastcdr::FastCdr object that can serialize/deserialize
     * the assigned buffer.
     *
     * @param cdr_buffer A reference to the buffer that contains (or will contain) the CDR representation.
     */
    FastCdr(
            FastBuffer& cdr_buffer);

    /*!
     * @brief This function skips a number of bytes in the CDR stream buffer.
     * @param num_bytes The number of bytes that will be jumped.
     * @return True is returned when the jump operation works successfully. Otherwise, false is returned.
     */
    bool jump(
            size_t num_bytes);

    /*!
     * @brief This function resets the current position in the buffer to the begining.
     */
    void reset();

    /*!
     * @brief This function returns the current position in the CDR stream.
     * @return Pointer to the current position in the buffer.
     */
    char* get_current_position();

    /*!
     * @brief This function returns the length of the serialized data inside the stream.
     * @return The length of the serialized data.
     */
    inline size_t get_serialized_data_length() const
    {
        return current_position_ - cdr_buffer_.begin();
    }

    /*!
     * @brief This function returns the current state of the CDR stream.
     * @return The current state of the buffer.
     */
    FastCdr::state get_state();

    /*!
     * @brief This function sets a previous state of the CDR stream;
     * @param state Previous state that will be set again.
     */
    void set_state(
            FastCdr::state& state);

    /*!
     * @brief This operator serializes an octet.
     * @param octet_t The value of the octet that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const uint8_t octet_t)
    {
        return serialize(octet_t);
    }

    /*!
     * @brief This operator serializes a character.
     * @param char_t The value of the character that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const char char_t)
    {
        return serialize(char_t);
    }

    /*!
     * @brief This operator serializes a int8_t.
     * @param int8 The value of the int8_t that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const int8_t int8)
    {
        return serialize(int8);
    }

    /*!
     * @brief This operator serializes an unsigned short.
     * @param ushort_t The value of the unsigned short that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const uint16_t ushort_t)
    {
        return serialize(ushort_t);
    }

    /*!
     * @brief This operator serializes a short.
     * @param short_t The value of the short that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const int16_t short_t)
    {
        return serialize(short_t);
    }

    /*!
     * @brief This operator serializes an unsigned long.
     * @param ulong_t The value of the unsigned long that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const uint32_t ulong_t)
    {
        return serialize(ulong_t);
    }

    /*!
     * @brief This operator serializes a long.
     * @param long_t The value of the long that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const int32_t long_t)
    {
        return serialize(long_t);
    }

    /*!
     * @brief This operator serializes a wide-char.
     * @param wchar The value of the wide-char that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const wchar_t wchar)
    {
        return serialize(wchar);
    }

    /*!
     * @brief This operator serializes an unsigned long long.
     * @param ulonglong_t The value of the unsigned long long that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const uint64_t ulonglong_t)
    {
        return serialize(ulonglong_t);
    }

    /*!
     * @brief This operator serializes a long long.
     * @param longlong_t The value of the long long that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const int64_t longlong_t)
    {
        return serialize(longlong_t);
    }

    /*!
     * @brief This operator serializes a float.
     * @param float_t The value of the float that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const float float_t)
    {
        return serialize(float_t);
    }

    /*!
     * @brief This operator serializes a ldouble.
     * @param double_t The value of the double that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const double double_t)
    {
        return serialize(double_t);
    }

    /*!
     * @brief This operator serializes a long double.
     * @param ldouble_t The value of the long double that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const long double ldouble_t)
    {
        return serialize(ldouble_t);
    }

    /*!
     * @brief This operator serializes a boolean.
     * @param bool_t The value of the boolean that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const bool bool_t)
    {
        return serialize(bool_t);
    }

    /*!
     * @brief This operator serializes a null-terminated string.
     * @param string_t The value of the string that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const char* string_t)
    {
        return serialize(string_t);
    }

    /*!
     * @brief This operator serializes a null-terminated wide-string.
     * @param string_t The value of the wide-string that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const wchar_t* string_t)
    {
        return serialize(string_t);
    }

    /*!
     * @brief This operator serializes a string.
     * @param string_t The string that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const std::string& string_t)
    {
        return serialize(string_t);
    }

    /*!
     * @brief This operator serializes a wstring.
     * @param string_t The wstring that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator <<(
            const std::wstring& string_t)
    {
        return serialize(string_t);
    }

    /*!
     * @brief This operator template is used to serialize arrays.
     * @param array_t The array that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T, size_t _Size>
    inline FastCdr& operator <<(
            const std::array<_T, _Size>& array_t)
    {
        return serialize<_T, _Size>(array_t);
    }

    /*!
     * @brief This operator template is used to serialize sequences.
     * @param vector_t The sequence that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    inline FastCdr& operator <<(
            const std::vector<_T>& vector_t)
    {
        return serialize<_T>(vector_t);
    }

    /*!
     * @brief This operator template is used to serialize non-basic types.
     * @param type_t The object that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    inline FastCdr& operator <<(
            const _T& type_t)
    {
        type_t.serialize(*this);
        return *this;
    }

    /*!
     * @brief This operator deserializes an octet.
     * @param octet_t The variable that will store the octet read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            uint8_t& octet_t)
    {
        return deserialize(octet_t);
    }

    /*!
     * @brief This operator deserializes a character.
     * @param char_t The variable that will store the character read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            char& char_t)
    {
        return deserialize(char_t);
    }

    /*!
     * @brief This operator deserializes an int8_t.
     * @param int8 The variable that will store the int8_t read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            int8_t& int8)
    {
        return deserialize(int8);
    }

    /*!
     * @brief This operator deserializes an unsigned short.
     * @param ushort_t The variable that will store the unsigned short read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            uint16_t& ushort_t)
    {
        return deserialize(ushort_t);
    }

    /*!
     * @brief This operator deserializes a short.
     * @param short_t The variable that will store the short read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            int16_t& short_t)
    {
        return deserialize(short_t);
    }

    /*!
     * @brief This operator deserializes an unsigned long.
     * @param ulong_t The variable that will store the unsigned long read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            uint32_t& ulong_t)
    {
        return deserialize(ulong_t);
    }

    /*!
     * @brief This operator deserializes a long.
     * @param long_t The variable that will store the long read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            int32_t& long_t)
    {
        return deserialize(long_t);
    }

    /*!
     * @brief This operator deserializes a wide-char.
     * @param wchar The variable that will store the wide-char read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            wchar_t& wchar)
    {
        return deserialize(wchar);
    }

    /*!
     * @brief This operator deserializes an unsigned long long.
     * @param ulonglong_t The variable that will store the unsigned long long read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            uint64_t& ulonglong_t)
    {
        return deserialize(ulonglong_t);
    }

    /*!
     * @brief This operator deserializes a long long.
     * @param longlong_t The variable that will store the long long read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            int64_t& longlong_t)
    {
        return deserialize(longlong_t);
    }

    /*!
     * @brief This operator deserializes a float.
     * @param float_t The variable that will store the float read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            float& float_t)
    {
        return deserialize(float_t);
    }

    /*!
     * @brief This operator deserializes a double.
     * @param double_t The variable that will store the double read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            double& double_t)
    {
        return deserialize(double_t);
    }

    /*!
     * @brief This operator deserializes a long double.
     * @param ldouble_t The variable that will store the long double read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            long double& ldouble_t)
    {
        return deserialize(ldouble_t);
    }

    /*!
     * @brief This operator deserializes a boolean.
     * @param bool_t The variable that will store the boolean read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     * @exception exception::BadParamException This exception is thrown when trying to deserialize in an invalid value.
     */
    inline FastCdr& operator >>(
            bool& bool_t)
    {
        return deserialize(bool_t);
    }

    /*!
     * @brief This operator deserializes a null-terminated c-string.
     * @param string_t The variable that will store the c-string read from the buffer.
     *                 Please note that a newly allocated string will be returned.
     *                 The caller should free the returned pointer when appropiate.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize a position that exceeds the internal memory size.
     * @exception exception::BadParamException This exception is thrown when trying to deserialize an invalid value.
     */
    inline FastCdr& operator >>(
            char*& string_t)
    {
        return deserialize(string_t);
    }

    /*!
     * @brief This operator deserializes a string.
     * @param string_t The variable that will store the string read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            std::string& string_t)
    {
        return deserialize(string_t);
    }

    /*!
     * @brief This operator deserializes a wstring.
     * @param string_t The variable that will store the wstring read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline FastCdr& operator >>(
            std::wstring& string_t)
    {
        return deserialize(string_t);
    }

    /*!
     * @brief This operator template is used to deserialize arrays.
     * @param array_t The variable that will store the array read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T, size_t _Size>
    inline FastCdr& operator >>(
            std::array<_T, _Size>& array_t)
    {
        return deserialize<_T, _Size>(array_t);
    }

    /*!
     * @brief This operator template is used to deserialize sequences.
     * @param vector_t The variable that will store the sequence read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    inline FastCdr& operator >>(
            std::vector<_T>& vector_t)
    {
        return deserialize<_T>(vector_t);
    }

    /*!
     * @brief This operator template is used to deserialize non-basic types.
     * @param type_t The variable that will store the object read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    inline FastCdr& operator >>(
            _T& type_t)
    {
        type_t.deserialize(*this);
        return *this;
    }

    /*!
     * @brief This function serializes an octet.
     * @param octet_t The value of the octet that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const uint8_t octet_t)
    {
        return serialize(static_cast<char>(octet_t));
    }

    /*!
     * @brief This function serializes a character.
     * @param char_t The value of the character that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const char char_t)
    {
        if (((last_position_ - current_position_) >= sizeof(char_t)) || resize(sizeof(char_t)))
        {
            current_position_++ << char_t;
            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function serializes an int8_t.
     * @param int8 The value of the int8_t that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const int8_t int8)
    {
        return serialize(static_cast<char>(int8));
    }

    /*!
     * @brief This function serializes an unsigned short.
     * @param ushort_t The value of the unsigned short that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const uint16_t ushort_t)
    {
        return serialize(static_cast<int16_t>(ushort_t));
    }

    /*!
     * @brief This function serializes a short.
     * @param short_t The value of the short that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const int16_t short_t)
    {
        if (((last_position_ - current_position_) >= sizeof(short_t)) || resize(sizeof(short_t)))
        {
            current_position_ << short_t;
            current_position_ += sizeof(short_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function serializes an unsigned long.
     * @param ulong_t The value of the unsigned long that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const uint32_t ulong_t)
    {
        return serialize(static_cast<int32_t>(ulong_t));
    }

    /*!
     * @brief This function serializes a long.
     * @param long_t The value of the long that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const int32_t long_t)
    {
        if (((last_position_ - current_position_) >= sizeof(long_t)) || resize(sizeof(long_t)))
        {
            current_position_ << long_t;
            current_position_ += sizeof(long_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function serializes a wide-char.
     * @param wchar The value of the wide-char that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const wchar_t wchar)
    {
        return serialize(static_cast<uint32_t>(wchar));
    }

    /*!
     * @brief This function serializes an unsigned long long.
     * @param ulonglong_t The value of the unsigned long long that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const uint64_t ulonglong_t)
    {
        return serialize(static_cast<int64_t>(ulonglong_t));
    }

    /*!
     * @brief This function serializes a long long.
     * @param longlong_t The value of the long long that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const int64_t longlong_t)
    {
        if (((last_position_ - current_position_) >= sizeof(longlong_t)) || resize(sizeof(longlong_t)))
        {
            current_position_ << longlong_t;
            current_position_ += sizeof(longlong_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function serializes a float.
     * @param float_t The value of the float that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const float float_t)
    {
        if (((last_position_ - current_position_) >= sizeof(float_t)) || resize(sizeof(float_t)))
        {
            current_position_ << float_t;
            current_position_ += sizeof(float_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function serializes a double.
     * @param double_t The value of the double that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const double double_t)
    {
        if (((last_position_ - current_position_) >= sizeof(double_t)) || resize(sizeof(double_t)))
        {
            current_position_ << double_t;
            current_position_ += sizeof(double_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function serializes a long double.
     * @param ldouble_t The value of the long double that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const long double ldouble_t)
    {
        if (((last_position_ - current_position_) >= sizeof(ldouble_t)) || resize(sizeof(ldouble_t)))
        {
            current_position_ << ldouble_t;
#if defined(_WIN32)
            current_position_ += sizeof(ldouble_t);
            current_position_ << static_cast<long double>(0);
#endif // if defined(_WIN32)
            current_position_ += sizeof(ldouble_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function serializes a boolean.
     * @param bool_t The value of the boolean that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize(
            const bool bool_t);

    /*!
     * @brief This function serializes a string.
     * @param string_t The pointer to the string that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize(
            const char* string_t);

    /*!
     * @brief This function serializes a wstring.
     * @param string_t The pointer to the wstring that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize(
            const wchar_t* string_t);

    /*!
     * @brief This function serializes a std::string.
     * @param string_t The string that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const std::string& string_t)
    {
        return serialize(string_t.c_str());
    }

    /*!
     * @brief This function serializes a std::wstring.
     * @param string_t The wstring that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize(
            const std::wstring& string_t)
    {
        return serialize(string_t.c_str());
    }

    /*!
     * @brief This function template serializes an array.
     * @param array_t The array that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T, size_t _Size>
    inline FastCdr& serialize(
            const std::array<_T, _Size>& array_t)
    {
        return serialize_array(array_t.data(), array_t.size());
    }

    /*!
     * @brief This function template serializes a sequence of booleans.
     * @param vector_t The sequence that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T = bool>
    FastCdr& serialize(
            const std::vector<bool>& vector_t)
    {
        return serialize_bool_sequence(vector_t);
    }

    /*!
     * @brief This function template serializes a sequence.
     * @param vector_t The sequence that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    FastCdr& serialize(
            const std::vector<_T>& vector_t)
    {
        state state_before_error(*this);

        *this << static_cast<int32_t>(vector_t.size());

        try
        {
            return serialize_array(vector_t.data(), vector_t.size());
        }
        catch (eprosima::fastcdr::exception::Exception& ex)
        {
            set_state(state_before_error);
            ex.raise();
        }

        return *this;
    }

#ifdef _MSC_VER
    /*!
     * @brief This function template serializes a sequence of booleans.
     * @param vector_t The sequence that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<>
    FastCdr& serialize<bool>(
            const std::vector<bool>& vector_t)
    {
        return serialize_bool_sequence(vector_t);
    }

#endif // ifdef _MSC_VER

    /*!
     * @brief This function template serializes a non-basic type.
     * @param type_t The object that will be serialized in the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    inline FastCdr& serialize(
            const _T& type_t)
    {
        type_t.serialize(*this);
        return *this;
    }

    /*!
     * @brief This function serializes an array of octets.
     * @param octet_t The sequence of octets that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize_array(
            const uint8_t* octet_t,
            size_t num_elements)
    {
        return serialize_array(reinterpret_cast<const char*>(octet_t), num_elements);
    }

    /*!
     * @brief This function serializes an array of characters.
     * @param char_t The array of characters that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize_array(
            const char* char_t,
            size_t num_elements);

    /*!
     * @brief This function serializes an array of int8_t.
     * @param int8 The sequence of int8_t that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize_array(
            const int8_t* int8,
            size_t num_elements)
    {
        return serialize_array(reinterpret_cast<const char*>(int8), num_elements);
    }

    /*!
     * @brief This function serializes an array of unsigned shorts.
     * @param ushort_t The array of unsigned shorts that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize_array(
            const uint16_t* ushort_t,
            size_t num_elements)
    {
        return serialize_array(reinterpret_cast<const int16_t*>(ushort_t), num_elements);
    }

    /*!
     * @brief This function serializes an array of shorts.
     * @param short_t The array of shorts that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize_array(
            const int16_t* short_t,
            size_t num_elements);

    /*!
     * @brief This function serializes an array of unsigned longs.
     * @param ulong_t The array of unsigned longs that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize_array(
            const uint32_t* ulong_t,
            size_t num_elements)
    {
        return serialize_array(reinterpret_cast<const int32_t*>(ulong_t), num_elements);
    }

    /*!
     * @brief This function serializes an array of longs.
     * @param long_t The array of longs that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize_array(
            const int32_t* long_t,
            size_t num_elements);

    /*!
     * @brief This function serializes an array of wide-chars.
     * @param wchar The array of wide-chars that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize_array(
            const wchar_t* wchar,
            size_t num_elements);

    /*!
     * @brief This function serializes an array of unsigned long longs.
     * @param ulonglong_t The array of unsigned long longs that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize_array(
            const uint64_t* ulonglong_t,
            size_t num_elements)
    {
        return serialize_array(reinterpret_cast<const int64_t*>(ulonglong_t), num_elements);
    }

    /*!
     * @brief This function serializes an array of long longs.
     * @param longlong_t The array of  long longs that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize_array(
            const int64_t* longlong_t,
            size_t num_elements);

    /*!
     * @brief This function serializes an array of floats.
     * @param float_t The array of floats that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize_array(
            const float* float_t,
            size_t num_elements);

    /*!
     * @brief This function serializes an array of doubles.
     * @param double_t The array of doubles that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize_array(
            const double* double_t,
            size_t num_elements);

    /*!
     * @brief This function serializes an array of long doubles.
     * @param ldouble_t The array of long doubles that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize_array(
            const long double* ldouble_t,
            size_t num_elements);

    /*!
     * @brief This function serializes an array of booleans.
     * @param bool_t The array of booleans that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    FastCdr& serialize_array(
            const bool* bool_t,
            size_t num_elements);

    /*!
     * @brief This function serializes an array of strings.
     * @param string_t The array of strings that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize_array(
            const std::string* string_t,
            size_t num_elements)
    {
        for (size_t count = 0; count < num_elements; ++count)
        {
            serialize(string_t[count].c_str());
        }
        return *this;
    }

    /*!
     * @brief This function serializes an array of wstrings.
     * @param string_t The array of wstrings that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& serialize_array(
            const std::wstring* string_t,
            size_t num_elements)
    {
        for (size_t count = 0; count < num_elements; ++count)
        {
            serialize(string_t[count].c_str());
        }
        return *this;
    }

    /*!
     * @brief This function template serializes an array of sequences.
     * @param vector_t The array of sequences that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    FastCdr& serialize_array(
            const std::vector<_T>* vector_t,
            size_t num_elements)
    {
        for (size_t count = 0; count < num_elements; ++count)
        {
            serialize(vector_t[count]);
        }
        return *this;
    }

    /*!
     * @brief This function template serializes an array of non-basic type objects.
     * @param type_t The array of objects that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    FastCdr& serialize_array(
            const _T* type_t,
            size_t num_elements)
    {
        for (size_t count = 0; count < num_elements; ++count)
        {
            type_t[count].serialize(*this);
        }
        return *this;
    }

    /*!
     * @brief This function template serializes a raw sequence.
     * @param sequence_t Pointer to the sequence that will be serialized in the buffer.
     * @param num_elements The number of elements contained in the sequence.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    FastCdr& serialize_sequence(
            const _T* sequence_t,
            size_t num_elements)
    {
        state state_before_error(*this);

        serialize(static_cast<int32_t>(num_elements));

        try
        {
            return serialize_array(sequence_t, num_elements);
        }
        catch (eprosima::fastcdr::exception::Exception& ex)
        {
            set_state(state_before_error);
            ex.raise();
        }

        return *this;
    }

    /*!
     * @brief This function deserializes an octet.
     * @param octet_t The variable that will store the octet read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            uint8_t& octet_t)
    {
        return deserialize(reinterpret_cast<char&>(octet_t));
    }

    /*!
     * @brief This function deserializes a character.
     * @param char_t The variable that will store the character read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            char& char_t)
    {
        if ((last_position_ - current_position_) >= sizeof(char_t))
        {
            current_position_++ >> char_t;
            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function deserializes an int8_t.
     * @param int8 The variable that will store the int8_t read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            int8_t& int8)
    {
        return deserialize(reinterpret_cast<char&>(int8));
    }

    /*!
     * @brief This function deserializes an unsigned short.
     * @param ushort_t The variable that will store the unsigned short read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            uint16_t& ushort_t)
    {
        return deserialize(reinterpret_cast<int16_t&>(ushort_t));
    }

    /*!
     * @brief This function deserializes a short.
     * @param short_t The variable that will store the short read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            int16_t& short_t)
    {
        if ((last_position_ - current_position_) >= sizeof(short_t))
        {
            current_position_ >> short_t;
            current_position_ += sizeof(short_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function deserializes an unsigned long.
     * @param ulong_t The variable that will store the unsigned long read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            uint32_t& ulong_t)
    {
        return deserialize(reinterpret_cast<int32_t&>(ulong_t));
    }

    /*!
     * @brief This function deserializes a long.
     * @param long_t The variable that will store the long read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            int32_t& long_t)
    {
        if ((last_position_ - current_position_) >= sizeof(long_t))
        {
            current_position_ >> long_t;
            current_position_ += sizeof(long_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function deserializes a wide-char.
     * @param wchar The variable that will store the wide-char read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            wchar_t& wchar)
    {
        uint32_t ret;
        deserialize(ret);
        wchar = static_cast<wchar_t>(ret);
        return *this;
    }

    /*!
     * @brief This function deserializes an unsigned long long.
     * @param ulonglong_t The variable that will store the unsigned long long read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            uint64_t& ulonglong_t)
    {
        return deserialize(reinterpret_cast<int64_t&>(ulonglong_t));
    }

    /*!
     * @brief This function deserializes a long long.
     * @param longlong_t The variable that will store the long long read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            int64_t& longlong_t)
    {
        if ((last_position_ - current_position_) >= sizeof(longlong_t))
        {
            current_position_ >> longlong_t;
            current_position_ += sizeof(longlong_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function deserializes a float.
     * @param float_t The variable that will store the float read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            float& float_t)
    {
        if ((last_position_ - current_position_) >= sizeof(float_t))
        {
            current_position_ >> float_t;
            current_position_ += sizeof(float_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function deserializes a double.
     * @param double_t The variable that will store the double read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            double& double_t)
    {
        if ((last_position_ - current_position_) >= sizeof(double_t))
        {
            current_position_ >> double_t;
            current_position_ += sizeof(double_t);

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function deserializes a long double.
     * @param ldouble_t The variable that will store the long double read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            long double& ldouble_t)
    {
        if ((last_position_ - current_position_) >= sizeof(ldouble_t))
        {
            current_position_ >> ldouble_t;
            current_position_ += sizeof(ldouble_t);
#if defined(_WIN32)
            current_position_ += sizeof(ldouble_t);
#endif // if defined(_WIN32)

            return *this;
        }

        throw exception::NotEnoughMemoryException(exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    /*!
     * @brief This function deserializes a boolean.
     * @param bool_t The variable that will store the boolean read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     * @exception exception::BadParamException This exception is thrown when trying to deserialize in an invalid value.
     */
    FastCdr& deserialize(
            bool& bool_t);

    /*!
     * @brief This function deserializes a string.
     * This function allocates memory to store the string. The user pointer will be set to point this allocated memory.
     * The user will have to free this allocated memory using free()
     * @param string_t The pointer that will point to the string read from the buffer.
     * The user will have to free the allocated memory using free()
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize(
            char*& string_t);

    /*!
     * @brief This function deserializes a wide string.
     * This function allocates memory to store the wide string. The user pointer will be set to point this allocated memory.
     * The user will have to free this allocated memory using free()
     * @param string_t The pointer that will point to the wide string read from the buffer.
     * The user will have to free the allocated memory using free()
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize(
            wchar_t*& string_t);

    /*!
     * @brief This function deserializes a std::string.
     * @param string_t The variable that will store the string read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            std::string& string_t)
    {
        uint32_t length = 0;
        const char* str = read_string(length);
        string_t = std::string(str, length);
        return *this;
    }

    /*!
     * @brief This function deserializes a std::wstring.
     * @param string_t The variable that will store the wstring read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize(
            std::wstring& string_t)
    {
        uint32_t length = 0;
        string_t = read_wstring(length);
        return *this;
    }

    /*!
     * @brief This function template deserializes an array.
     * @param array_t The variable that will store the array read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T, size_t _Size>
    inline FastCdr& deserialize(
            std::array<_T, _Size>& array_t)
    {
        return deserialize_array(array_t.data(), array_t.size());
    }

    /*!
     * @brief This function template deserializes a sequence of booleans.
     * @param vector_t The variable that will store the sequence read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T = bool>
    FastCdr& deserialize(
            std::vector<bool>& vector_t)
    {
        return deserialize_bool_sequence(vector_t);
    }

    /*!
     * @brief This function template deserializes a sequence.
     * @param vector_t The variable that will store the sequence read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    FastCdr& deserialize(
            std::vector<_T>& vector_t)
    {
        uint32_t sequence_length = 0;
        state state_before_error(*this);

        *this >> sequence_length;

        try
        {
            vector_t.resize(sequence_length);
            return deserialize_array(vector_t.data(), vector_t.size());
        }
        catch (eprosima::fastcdr::exception::Exception& ex)
        {
            set_state(state_before_error);
            ex.raise();
        }

        return *this;
    }

#ifdef _MSC_VER
    /*!
     * @brief This function template deserializes a sequence of booleans.
     * @param vector_t The variable that will store the sequence read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<>
    FastCdr& deserialize<bool>(
            std::vector<bool>& vector_t)
    {
        return deserialize_bool_sequence(vector_t);
    }

#endif // ifdef _MSC_VER

    /*!
     * @brief This function template deserializes a non-basic type object.
     * @param type_t The variable that will store the object read from the buffer.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    inline FastCdr& deserialize(
            _T& type_t)
    {
        type_t.deserialize(*this);
        return *this;
    }

    /*!
     * @brief This function deserializes an array of octets.
     * @param octet_t The variable that will store the array of octets read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize_array(
            uint8_t* octet_t,
            size_t num_elements)
    {
        return deserialize_array(reinterpret_cast<char*>(octet_t), num_elements);
    }

    /*!
     * @brief This function deserializes an array of characters.
     * @param char_t The variable that will store the array of characters read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize_array(
            char* char_t,
            size_t num_elements);

    /*!
     * @brief This function deserializes an array of int8_t.
     * @param int8 The variable that will store the array of int8_t read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize_array(
            int8_t* int8,
            size_t num_elements)
    {
        return deserialize_array(reinterpret_cast<char*>(int8), num_elements);
    }

    /*!
     * @brief This function deserializes an array of unsigned shorts.
     * @param ushort_t The variable that will store the array of unsigned shorts read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize_array(
            uint16_t* ushort_t,
            size_t num_elements)
    {
        return deserialize_array(reinterpret_cast<int16_t*>(ushort_t), num_elements);
    }

    /*!
     * @brief This function deserializes an array of shorts.
     * @param short_t The variable that will store the array of shorts read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize_array(
            int16_t* short_t,
            size_t num_elements);

    /*!
     * @brief This function deserializes an array of unsigned longs.
     * @param ulong_t The variable that will store the array of unsigned longs read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize_array(
            uint32_t* ulong_t,
            size_t num_elements)
    {
        return deserialize_array(reinterpret_cast<int32_t*>(ulong_t), num_elements);
    }

    /*!
     * @brief This function deserializes an array of longs.
     * @param long_t The variable that will store the array of longs read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize_array(
            int32_t* long_t,
            size_t num_elements);

    /*!
     * @brief This function deserializes an array of wide-chars.
     * @param wchar The variable that will store the array of wide-chars read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize_array(
            wchar_t* wchar,
            size_t num_elements);

    /*!
     * @brief This function deserializes an array of unsigned long longs.
     * @param ulonglong_t The variable that will store the array of unsigned long longs read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize_array(
            uint64_t* ulonglong_t,
            size_t num_elements)
    {
        return deserialize_array(reinterpret_cast<int64_t*>(ulonglong_t), num_elements);
    }

    /*!
     * @brief This function deserializes an array of long longs.
     * @param longlong_t The variable that will store the array of long longs read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize_array(
            int64_t* longlong_t,
            size_t num_elements);

    /*!
     * @brief This function deserializes an array of floats.
     * @param float_t The variable that will store the array of floats read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize_array(
            float* float_t,
            size_t num_elements);

    /*!
     * @brief This function deserializes an array of doubles.
     * @param double_t The variable that will store the array of doubles read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize_array(
            double* double_t,
            size_t num_elements);

    /*!
     * @brief This function deserializes an array of long doubles.
     * @param ldouble_t The variable that will store the array of long doubles read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize_array(
            long double* ldouble_t,
            size_t num_elements);

    /*!
     * @brief This function deserializes an array of booleans.
     * @param bool_t The variable that will store the array of booleans read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    FastCdr& deserialize_array(
            bool* bool_t,
            size_t num_elements);

    /*!
     * @brief This function deserializes an array of strings.
     * @param string_t The variable that will store the array of strings read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize_array(
            std::string* string_t,
            size_t num_elements)
    {
        for (size_t count = 0; count < num_elements; ++count)
        {
            deserialize(string_t[count]);
        }
        return *this;
    }

    /*!
     * @brief This function deserializes an array of wide-strings.
     * @param string_t The variable that will store the array of strings read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    inline
    FastCdr& deserialize_array(
            std::wstring* string_t,
            size_t num_elements)
    {
        for (size_t count = 0; count < num_elements; ++count)
        {
            deserialize(string_t[count]);
        }
        return *this;
    }

    /*!
     * @brief This function template deserializes an array of sequences.
     * @param vector_t The variable that will store the array of sequences read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    FastCdr& deserialize_array(
            std::vector<_T>* vector_t,
            size_t num_elements)
    {
        for (size_t count = 0; count < num_elements; ++count)
        {
            deserialize(vector_t[count]);
        }
        return *this;
    }

    /*!
     * @brief This function template deserializes an array of non-basic type objects.
     * @param type_t The variable that will store the array of objects read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    FastCdr& deserialize_array(
            _T* type_t,
            size_t num_elements)
    {
        for (size_t count = 0; count < num_elements; ++count)
        {
            type_t[count].deserialize(*this);
        }
        return *this;
    }

    /*!
     * @brief This function template deserializes a string sequence.
     * This function allocates memory to store the sequence. The user pointer will be set to point this allocated memory.
     * The user will have to free this allocated memory using free()
     * @param sequence_t The pointer that will store the sequence read from the buffer.
     * @param num_elements This variable return the number of elements of the sequence.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T = std::string>
    FastCdr& deserialize_sequence(
            std::string*& sequence_t,
            size_t& num_elements)
    {
        return deserialize_string_sequence(sequence_t, num_elements);
    }

    /*!
     * @brief This function template deserializes a wide-string sequence.
     * This function allocates memory to store the sequence. The user pointer will be set to point this allocated memory.
     * The user will have to free this allocated memory using free()
     * @param sequence_t The pointer that will store the sequence read from the buffer.
     * @param num_elements This variable return the number of elements of the sequence.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T = std::wstring>
    FastCdr& deserialize_sequence(
            std::wstring*& sequence_t,
            size_t& num_elements)
    {
        return deserialize_wstring_sequence(sequence_t, num_elements);
    }

    /*!
     * @brief This function template deserializes a raw sequence.
     * This function allocates memory to store the sequence. The user pointer will be set to point this allocated memory.
     * The user will have to free this allocated memory using free()
     * @param sequence_t The pointer that will store the sequence read from the buffer.
     * @param num_elements This variable return the number of elements of the sequence.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T>
    FastCdr& deserialize_sequence(
            _T*& sequence_t,
            size_t& num_elements)
    {
        uint32_t sequence_length = 0;
        state state_before_error(*this);

        deserialize(sequence_length);

        try
        {
            sequence_t = reinterpret_cast<_T*>(calloc(sequence_length, sizeof(_T)));
            deserialize_array(sequence_t, sequence_length);
        }
        catch (eprosima::fastcdr::exception::Exception& ex)
        {
            free(sequence_t);
            sequence_t = NULL;
            set_state(state_before_error);
            ex.raise();
        }

        num_elements = sequence_length;
        return *this;
    }

#ifdef _MSC_VER
    /*!
     * @brief This function template deserializes a string sequence.
     * This function allocates memory to store the sequence. The user pointer will be set to point this allocated memory.
     * The user will have to free this allocated memory using free()
     * @param sequence_t The pointer that will store the sequence read from the buffer.
     * @param num_elements This variable return the number of elements of the sequence.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<>
    FastCdr& deserialize_sequence<std::string>(
            std::string*& sequence_t,
            size_t& num_elements)
    {
        return deserialize_string_sequence(sequence_t, num_elements);
    }

    /*!
     * @brief This function template deserializes a wide-string sequence.
     * This function allocates memory to store the sequence. The user pointer will be set to point this allocated memory.
     * The user will have to free this allocated memory using free()
     * @param sequence_t The pointer that will store the sequence read from the buffer.
     * @param num_elements This variable return the number of elements of the sequence.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<>
    FastCdr& deserialize_sequence<std::wstring>(
            std::wstring*& sequence_t,
            size_t& num_elements)
    {
        return deserialize_wstring_sequence(sequence_t, num_elements);
    }

#endif // ifdef _MSC_VER

private:

    FastCdr(
            const FastCdr&) = delete;

    FastCdr& operator =(
            const FastCdr&) = delete;

    FastCdr& serialize_bool_sequence(
            const std::vector<bool>& vector_t);

    FastCdr& deserialize_bool_sequence(
            std::vector<bool>& vector_t);

    FastCdr& deserialize_string_sequence(
            std::string*& sequence_t,
            size_t& num_elements);

    FastCdr& deserialize_wstring_sequence(
            std::wstring*& sequence_t,
            size_t& num_elements);

    /*!
     * @brief This function template detects the content type of the STD container array and serializes the array.
     * @param array_t The array that will be serialized in the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to serialize in a position that exceeds the internal memory size.
     */
    template<class _T, size_t _Size>
    FastCdr& serialize_array(
            const std::array<_T, _Size>* array_t,
            size_t num_elements)
    {
        return serialize_array(array_t->data(), num_elements * array_t->size());
    }

    /*!
     * @brief This function template detects the content type of the STD container array and deserializes the array.
     * @param array_t The variable that will store the array read from the buffer.
     * @param num_elements Number of the elements in the array.
     * @return Reference to the eprosima::fastcdr::FastCdr object.
     * @exception exception::NotEnoughMemoryException This exception is thrown when trying to deserialize in a position that exceeds the internal memory size.
     */
    template<class _T, size_t _Size>
    FastCdr& deserialize_array(
            std::array<_T, _Size>* array_t,
            size_t num_elements)
    {
        return deserialize_array(array_t->data(), num_elements * array_t->size());
    }

    bool resize(
            size_t min_size_inc);

    const char* read_string(
            uint32_t& length);

    std::wstring read_wstring(
            uint32_t& length);

    //! @brief Reference to the buffer that will be serialized/deserialized.
    FastBuffer& cdr_buffer_;

    //! @brief The current position in the serialization/deserialization process.
    FastBuffer::iterator current_position_;

    //! @brief The last position in the buffer;
    FastBuffer::iterator last_position_;
};
}     //namespace fastcdr
} //namespace eprosima

#endif //_FASTCDR_FASTCDR_H_
