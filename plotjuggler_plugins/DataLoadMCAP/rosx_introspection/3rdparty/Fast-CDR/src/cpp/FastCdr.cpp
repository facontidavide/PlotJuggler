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

#include <fastcdr/FastCdr.h>
#include <fastcdr/exceptions/BadParamException.h>
#include <string.h>

using namespace eprosima::fastcdr;
using namespace ::exception;

FastCdr::state::state(
        const FastCdr& fastcdr)
    : m_currentPosition(fastcdr.m_currentPosition)
{
}

FastCdr::state::state(
        const state& current_state)
    : m_currentPosition(current_state.m_currentPosition)
{
}

FastCdr::FastCdr(
        FastBuffer& cdrBuffer)
    : m_cdrBuffer(cdrBuffer)
    , m_currentPosition(cdrBuffer.begin())
    , m_lastPosition(cdrBuffer.end())
{
}

bool FastCdr::jump(
        size_t numBytes)
{
    bool returnedValue = false;

    if (((m_lastPosition - m_currentPosition) >= numBytes) || resize(numBytes))
    {
        m_currentPosition += numBytes;
        returnedValue = true;
    }

    return returnedValue;
}

char* FastCdr::getCurrentPosition()
{
    return &m_currentPosition;
}

FastCdr::state FastCdr::getState()
{
    return FastCdr::state(*this);
}

void FastCdr::setState(
        FastCdr::state& current_state)
{
    m_currentPosition >> current_state.m_currentPosition;
}

void FastCdr::reset()
{
    m_currentPosition = m_cdrBuffer.begin();
}

bool FastCdr::resize(
        size_t minSizeInc)
{
    if (m_cdrBuffer.resize(minSizeInc))
    {
        m_currentPosition << m_cdrBuffer.begin();
        m_lastPosition = m_cdrBuffer.end();
        return true;
    }

    return false;
}

FastCdr& FastCdr::serialize(
        const bool bool_t)
{
    uint8_t value = 0;

    if (((m_lastPosition - m_currentPosition) >= sizeof(uint8_t)) || resize(sizeof(uint8_t)))
    {
        if (bool_t)
        {
            value = 1;
        }
        m_currentPosition++ << value;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::serialize(
        const char* string_t)
{
    uint32_t length = 0;

    if (string_t != nullptr)
    {
        length = size_to_uint32(strlen(string_t)) + 1;
    }

    if (length > 0)
    {
        FastCdr::state state_before_error(*this);
        serialize(length);

        if (((m_lastPosition - m_currentPosition) >= length) || resize(length))
        {
            m_currentPosition.memcopy(string_t, length);
            m_currentPosition += length;
        }
        else
        {
            setState(state_before_error);
            throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
        }
    }
    else
    {
        serialize(length);
    }

    return *this;
}

FastCdr& FastCdr::serialize(
        const wchar_t* string_t)
{
    uint32_t bytesLength = 0;
    size_t wstrlen = 0;

    if (string_t != nullptr)
    {
        wstrlen = wcslen(string_t);
        bytesLength = size_to_uint32(wstrlen * 4);
    }

    if (bytesLength > 0)
    {
        FastCdr::state state_(*this);
        serialize(size_to_uint32(wstrlen));

        if (((m_lastPosition - m_currentPosition) >= bytesLength) || resize(bytesLength))
        {
#if defined(_WIN32)
            serializeArray(string_t, wstrlen);
#else
            m_currentPosition.memcopy(string_t, bytesLength);
            m_currentPosition += bytesLength; // size on bytes
#endif // if defined(_WIN32)
        }
        else
        {
            setState(state_);
            throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
        }
    }
    else
    {
        serialize(bytesLength);
    }

    return *this;
}

FastCdr& FastCdr::serializeArray(
        const bool* bool_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*bool_t) * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        for (size_t count = 0; count < numElements; ++count)
        {
            uint8_t value = 0;

            if (bool_t[count])
            {
                value = 1;
            }
            m_currentPosition++ << value;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::serializeArray(
        const char* char_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*char_t) * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        m_currentPosition.memcopy(char_t, totalSize);
        m_currentPosition += totalSize;
        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::serializeArray(
        const int16_t* short_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*short_t) * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        m_currentPosition.memcopy(short_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::serializeArray(
        const int32_t* long_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*long_t) * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        m_currentPosition.memcopy(long_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::serializeArray(
        const wchar_t* wchar,
        size_t numElements)
{
    for (size_t count = 0; count < numElements; ++count)
    {
        serialize(wchar[count]);
    }
    return *this;
}

FastCdr& FastCdr::serializeArray(
        const int64_t* longlong_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*longlong_t) * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        m_currentPosition.memcopy(longlong_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::serializeArray(
        const float* float_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*float_t) * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        m_currentPosition.memcopy(float_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::serializeArray(
        const double* double_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*double_t) * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        m_currentPosition.memcopy(double_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::serializeArray(
        const long double* ldouble_t,
        size_t numElements)
{
    size_t totalSize = 16 * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
#if FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
        for (size_t idx = 0; idx < numElements; ++idx)
        {
            __float128 tmp = ldouble_t[idx];
            m_currentPosition << tmp;
            m_currentPosition += 16;
        }
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 16
        m_currentPosition.memcopy(ldouble_t, totalSize);
        m_currentPosition += totalSize;
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8
        for (size_t idx = 0; idx < numElements; ++idx)
        {
            m_currentPosition << static_cast<long double>(0);
            m_currentPosition += 8;
            m_currentPosition << ldouble_t[idx];
            m_currentPosition += 8;
        }
#else
#error unsupported long double type and no __float128 available
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 16
#endif // FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserialize(
        bool& bool_t)
{
    uint8_t value = 0;

    if ((m_lastPosition - m_currentPosition) >= sizeof(uint8_t))
    {
        m_currentPosition++ >> value;

        if (value == 1)
        {
            bool_t = true;
            return *this;
        }
        else if (value == 0)
        {
            bool_t = false;
            return *this;
        }

        throw BadParamException("Got unexpected byte value in deserialize for bool (expected 0 or 1)");
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserialize(
        char*& string_t)
{
    uint32_t length = 0;
    FastCdr::state state_before_error(*this);

    deserialize(length);

    if (length == 0)
    {
        string_t = NULL;
        return *this;
    }
    else if ((m_lastPosition - m_currentPosition) >= length)
    {
        // Allocate memory.
        string_t =
                reinterpret_cast<char*>(calloc(length + ((&m_currentPosition)[length - 1] == '\0' ? 0 : 1),
                sizeof(char)));
        memcpy(string_t, &m_currentPosition, length);
        m_currentPosition += length;
        return *this;
    }

    setState(state_before_error);
    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserialize(
        wchar_t*& string_t)
{
    uint32_t length = 0;
    FastCdr::state state_before_error(*this);

    deserialize(length);

    if (length == 0)
    {
        string_t = NULL;
        return *this;
    }
    else if ((m_lastPosition - m_currentPosition) >= length)
    {
        // Allocate memory.
        string_t = reinterpret_cast<wchar_t*>(calloc(length + 1, sizeof(wchar_t))); // WStrings never serialize terminating zero

#if defined(_WIN32)
        for (size_t idx = 0; idx < length; ++idx)
        {
            uint32_t temp;
            m_currentPosition >> temp;
            string_t[idx] = static_cast<wchar_t>(temp);
            m_currentPosition += 4;
        }
#else
        memcpy(string_t, &m_currentPosition, length * sizeof(wchar_t));
        m_currentPosition += length * sizeof(wchar_t);
#endif // if defined(_WIN32)

        return *this;
    }

    setState(state_before_error);
    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

const char* FastCdr::readString(
        uint32_t& length)
{
    const char* returnedValue = "";
    state state_before_error(*this);

    *this >> length;

    if (length == 0)
    {
        return returnedValue;
    }
    else if ((m_lastPosition - m_currentPosition) >= length)
    {
        returnedValue = &m_currentPosition;
        m_currentPosition += length;
        if (returnedValue[length - 1] == '\0')
        {
            --length;
        }
        return returnedValue;
    }

    setState(state_before_error);
    throw eprosima::fastcdr::exception::NotEnoughMemoryException(
              eprosima::fastcdr::exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

std::wstring FastCdr::readWString(
        uint32_t& length)
{
    std::wstring returnedValue = L"";
    state state_(*this);

    *this >> length;
    uint32_t bytesLength = length * 4;

    if (bytesLength == 0)
    {
        return returnedValue;
    }
    else if ((m_lastPosition - m_currentPosition) >= bytesLength)
    {

        returnedValue.resize(length);
        deserializeArray(const_cast<wchar_t*>(returnedValue.c_str()), length);
        if (returnedValue[length - 1] == L'\0')
        {
            --length;
            returnedValue.erase(length);
        }
        return returnedValue;
    }

    setState(state_);
    throw eprosima::fastcdr::exception::NotEnoughMemoryException(
              eprosima::fastcdr::exception::NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserializeArray(
        bool* bool_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*bool_t) * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        for (size_t count = 0; count < numElements; ++count)
        {
            uint8_t value = 0;
            m_currentPosition++ >> value;

            if (value == 1)
            {
                bool_t[count] = true;
            }
            else if (value == 0)
            {
                bool_t[count] = false;
            }
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserializeArray(
        char* char_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*char_t) * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        m_currentPosition.rmemcopy(char_t, totalSize);
        m_currentPosition += totalSize;
        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserializeArray(
        int16_t* short_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*short_t) * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        m_currentPosition.rmemcopy(short_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserializeArray(
        int32_t* long_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*long_t) * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        m_currentPosition.rmemcopy(long_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserializeArray(
        wchar_t* wchar,
        size_t numElements)
{
    uint32_t value = 0;
    for (size_t count = 0; count < numElements; ++count)
    {
        deserialize(value);
        wchar[count] = static_cast<wchar_t>(value);
    }
    return *this;
}

FastCdr& FastCdr::deserializeArray(
        int64_t* longlong_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*longlong_t) * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        m_currentPosition.rmemcopy(longlong_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserializeArray(
        float* float_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*float_t) * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        m_currentPosition.rmemcopy(float_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserializeArray(
        double* double_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*double_t) * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        m_currentPosition.rmemcopy(double_t, totalSize);
        m_currentPosition += totalSize;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::deserializeArray(
        long double* ldouble_t,
        size_t numElements)
{
    size_t totalSize = 16 * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
#if FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
        for (size_t idx = 0; idx < numElements; ++idx)
        {
            __float128 tmp;
            m_currentPosition >> tmp;
            m_currentPosition += 16;
            ldouble_t[idx] = static_cast<long double>(tmp);
        }
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 16
        m_currentPosition.rmemcopy(ldouble_t, totalSize);
        m_currentPosition += totalSize;
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8
        for (size_t idx = 0; idx < numElements; ++idx)
        {
            m_currentPosition += 8;
            m_currentPosition >> ldouble_t[idx];
            m_currentPosition += 8;
        }
#else
#error unsupported long double type and no __float128 available
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 16
#endif // FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

FastCdr& FastCdr::serializeBoolSequence(
        const std::vector<bool>& vector_t)
{
    state state_before_error(*this);

    *this << static_cast<int32_t>(vector_t.size());

    size_t totalSize = vector_t.size() * sizeof(bool);

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        for (size_t count = 0; count < vector_t.size(); ++count)
        {
            uint8_t value = 0;
            std::vector<bool>::const_reference ref = vector_t[count];

            if (ref)
            {
                value = 1;
            }
            m_currentPosition++ << value;
        }
    }
    else
    {
        setState(state_before_error);
        throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    return *this;
}

FastCdr& FastCdr::deserializeBoolSequence(
        std::vector<bool>& vector_t)
{
    uint32_t seqLength = 0;
    state state_before_error(*this);

    *this >> seqLength;

    size_t totalSize = seqLength * sizeof(bool);

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        vector_t.resize(seqLength);
        for (uint32_t count = 0; count < seqLength; ++count)
        {
            uint8_t value = 0;
            m_currentPosition++ >> value;

            if (value == 1)
            {
                vector_t[count] = true;
            }
            else if (value == 0)
            {
                vector_t[count] = false;
            }
        }
    }
    else
    {
        setState(state_before_error);
        throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
    }

    return *this;
}

FastCdr& FastCdr::deserializeStringSequence(
        std::string*& sequence_t,
        size_t& numElements)
{
    uint32_t seqLength = 0;
    state state_before_error(*this);

    deserialize(seqLength);

    try
    {
        sequence_t = new std::string[seqLength];
        deserializeArray(sequence_t, seqLength);
    }
    catch (eprosima::fastcdr::exception::Exception& ex)
    {
        delete [] sequence_t;
        sequence_t = NULL;
        setState(state_before_error);
        ex.raise();
    }

    numElements = seqLength;
    return *this;
}

FastCdr& FastCdr::deserializeWStringSequence(
        std::wstring*& sequence_t,
        size_t& numElements)
{
    uint32_t seqLength = 0;
    state state_before_error(*this);

    deserialize(seqLength);

    try
    {
        sequence_t = new std::wstring[seqLength];
        deserializeArray(sequence_t, seqLength);
    }
    catch (eprosima::fastcdr::exception::Exception& ex)
    {
        delete [] sequence_t;
        sequence_t = NULL;
        setState(state_before_error);
        ex.raise();
    }

    numElements = seqLength;
    return *this;
}
