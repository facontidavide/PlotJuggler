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

#include <fastcdr/Cdr.h>
#include <fastcdr/exceptions/BadParamException.h>

using namespace eprosima::fastcdr;
using namespace ::exception;

#if FASTCDR_IS_BIG_ENDIAN_TARGET
const Cdr::Endianness Cdr::DEFAULT_ENDIAN = BIG_ENDIANNESS;
#else
const Cdr::Endianness Cdr::DEFAULT_ENDIAN = LITTLE_ENDIANNESS;
#endif // if FASTCDR_IS_BIG_ENDIAN_TARGET

CONSTEXPR size_t ALIGNMENT_LONG_DOUBLE = 8;

Cdr::state::state(
        const Cdr& cdr)
    : m_currentPosition(cdr.m_currentPosition)
    , m_alignPosition(cdr.m_alignPosition)
    , m_swapBytes(cdr.m_swapBytes)
    , m_lastDataSize(cdr.m_lastDataSize)
{
}

Cdr::state::state(
        const state& current_state)
    : m_currentPosition(current_state.m_currentPosition)
    , m_alignPosition(current_state.m_alignPosition)
    , m_swapBytes(current_state.m_swapBytes)
    , m_lastDataSize(current_state.m_lastDataSize)
{
}

Cdr::Cdr(
        FastBuffer& cdrBuffer,
        const Endianness endianness,
        const CdrType cdrType)
    : m_cdrBuffer(cdrBuffer)
    , m_cdrType(cdrType)
    , m_plFlag(DDS_CDR_WITHOUT_PL)
    , m_options(0)
    , m_endianness(static_cast<uint8_t>(endianness))
    , m_swapBytes(endianness == DEFAULT_ENDIAN ? false : true)
    , m_lastDataSize(0)
    , m_currentPosition(cdrBuffer.begin())
    , m_alignPosition(cdrBuffer.begin())
    , m_lastPosition(cdrBuffer.end())
{
}

Cdr& Cdr::read_encapsulation()
{
    uint8_t dummy = 0, encapsulationKind = 0;
    state state_before_error(*this);

    try
    {
        // If it is DDS_CDR, the first step is to get the dummy byte.
        if (m_cdrType == DDS_CDR)
        {
            (*this) >> dummy;
            if (0 != dummy)
            {
                throw BadParamException("Unexpected non-zero initial byte received in Cdr::read_encapsulation");
            }
        }

        // Get the ecampsulation byte.
        (*this) >> encapsulationKind;


        // If it is a different endianness, make changes.
        if (m_endianness != (encapsulationKind & 0x1))
        {
            m_swapBytes = !m_swapBytes;
            m_endianness = (encapsulationKind & 0x1);
        }

        // Check encapsulationKind correctness
        uint8_t allowed_kind_mask = LITTLE_ENDIANNESS;
        if (m_cdrType == DDS_CDR)
        {
            allowed_kind_mask |= DDS_CDR_WITH_PL;
        }

        if (0 != (encapsulationKind & ~allowed_kind_mask))
        {
            throw BadParamException("Unexpected CDR type received in Cdr::read_encapsulation");
        }

        // If it is DDS_CDR type, view if contains a parameter list.
        if ((encapsulationKind & DDS_CDR_WITH_PL) && ((m_cdrType == DDS_CDR)))
        {
            m_plFlag = DDS_CDR_WITH_PL;
        }

        if (m_cdrType == DDS_CDR)
        {
            (*this) >> m_options;
        }
    }
    catch (Exception& ex)
    {
        setState(state_before_error);
        ex.raise();
    }

    resetAlignment();
    return *this;
}

Cdr& Cdr::serialize_encapsulation()
{
    uint8_t dummy = 0, encapsulationKind = 0;
    state state_before_error(*this);

    try
    {
        // If it is DDS_CDR, the first step is to serialize the dummy byte.
        if (m_cdrType == DDS_CDR)
        {
            (*this) << dummy;
        }

        // Construct encapsulation byte.
        encapsulationKind = (static_cast<uint8_t>(m_plFlag) | m_endianness);

        // Serialize the encapsulation byte.
        (*this) << encapsulationKind;
    }
    catch (Exception& ex)
    {
        setState(state_before_error);
        ex.raise();
    }

    try
    {
        if (m_cdrType == DDS_CDR)
        {
            (*this) << m_options;
        }
    }
    catch (Exception& ex)
    {
        setState(state_before_error);
        ex.raise();
    }

    resetAlignment();
    return *this;
}

Cdr::DDSCdrPlFlag Cdr::getDDSCdrPlFlag() const
{
    return m_plFlag;
}

void Cdr::setDDSCdrPlFlag(
        DDSCdrPlFlag plFlag)
{
    m_plFlag = plFlag;
}

uint16_t Cdr::getDDSCdrOptions() const
{
    return m_options;
}

void Cdr::setDDSCdrOptions(
        uint16_t options)
{
    m_options = options;
}

void Cdr::changeEndianness(
        Endianness endianness)
{
    if (m_endianness != endianness)
    {
        m_swapBytes = !m_swapBytes;
        m_endianness = endianness;
    }
}

bool Cdr::jump(
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

char* Cdr::getBufferPointer()
{
    return m_cdrBuffer.getBuffer();
}

char* Cdr::getCurrentPosition()
{
    return &m_currentPosition;
}

Cdr::state Cdr::getState()
{
    return Cdr::state(*this);
}

void Cdr::setState(
        state& current_state)
{
    m_currentPosition >> current_state.m_currentPosition;
    m_alignPosition >> current_state.m_alignPosition;
    m_swapBytes = current_state.m_swapBytes;
    m_lastDataSize = current_state.m_lastDataSize;
}

void Cdr::reset()
{
    m_currentPosition = m_cdrBuffer.begin();
    m_alignPosition = m_cdrBuffer.begin();
    m_swapBytes = m_endianness == DEFAULT_ENDIAN ? false : true;
    m_lastDataSize = 0;
}

bool Cdr::moveAlignmentForward(
        size_t numBytes)
{
    bool returnedValue = false;

    if (((m_lastPosition - m_alignPosition) >= numBytes) || resize(numBytes))
    {
        m_alignPosition += numBytes;
        returnedValue = true;
    }

    return returnedValue;
}

bool Cdr::resize(
        size_t minSizeInc)
{
    if (m_cdrBuffer.resize(minSizeInc))
    {
        m_currentPosition << m_cdrBuffer.begin();
        m_alignPosition << m_cdrBuffer.begin();
        m_lastPosition = m_cdrBuffer.end();
        return true;
    }

    return false;
}

Cdr& Cdr::serialize(
        const char char_t)
{
    if (((m_lastPosition - m_currentPosition) >= sizeof(char_t)) || resize(sizeof(char_t)))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(char_t);

        m_currentPosition++ << char_t;
        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serialize(
        const int16_t short_t)
{
    size_t align = alignment(sizeof(short_t));
    size_t sizeAligned = sizeof(short_t) + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(short_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&short_t);

            m_currentPosition++ << dst[1];
            m_currentPosition++ << dst[0];
        }
        else
        {
            m_currentPosition << short_t;
            m_currentPosition += sizeof(short_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serialize(
        const int16_t short_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serialize(short_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serialize(
        const int32_t long_t)
{
    size_t align = alignment(sizeof(long_t));
    size_t sizeAligned = sizeof(long_t) + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(long_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&long_t);

            m_currentPosition++ << dst[3];
            m_currentPosition++ << dst[2];
            m_currentPosition++ << dst[1];
            m_currentPosition++ << dst[0];
        }
        else
        {
            m_currentPosition << long_t;
            m_currentPosition += sizeof(long_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serialize(
        const int32_t long_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serialize(long_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serialize(
        const int64_t longlong_t)
{
    size_t align = alignment(sizeof(longlong_t));
    size_t sizeAligned = sizeof(longlong_t) + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(longlong_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&longlong_t);

            m_currentPosition++ << dst[7];
            m_currentPosition++ << dst[6];
            m_currentPosition++ << dst[5];
            m_currentPosition++ << dst[4];
            m_currentPosition++ << dst[3];
            m_currentPosition++ << dst[2];
            m_currentPosition++ << dst[1];
            m_currentPosition++ << dst[0];
        }
        else
        {
            m_currentPosition << longlong_t;
            m_currentPosition += sizeof(longlong_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serialize(
        const int64_t longlong_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serialize(longlong_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serialize(
        const float float_t)
{
    size_t align = alignment(sizeof(float_t));
    size_t sizeAligned = sizeof(float_t) + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(float_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&float_t);

            m_currentPosition++ << dst[3];
            m_currentPosition++ << dst[2];
            m_currentPosition++ << dst[1];
            m_currentPosition++ << dst[0];
        }
        else
        {
            m_currentPosition << float_t;
            m_currentPosition += sizeof(float_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serialize(
        const float float_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serialize(float_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serialize(
        const double double_t)
{
    size_t align = alignment(sizeof(double_t));
    size_t sizeAligned = sizeof(double_t) + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(double_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&double_t);

            m_currentPosition++ << dst[7];
            m_currentPosition++ << dst[6];
            m_currentPosition++ << dst[5];
            m_currentPosition++ << dst[4];
            m_currentPosition++ << dst[3];
            m_currentPosition++ << dst[2];
            m_currentPosition++ << dst[1];
            m_currentPosition++ << dst[0];
        }
        else
        {
            m_currentPosition << double_t;
            m_currentPosition += sizeof(double_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serialize(
        const double double_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serialize(double_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serialize(
        const long double ldouble_t)
{
    size_t align = alignment(ALIGNMENT_LONG_DOUBLE);
    size_t sizeAligned = sizeof(ldouble_t) + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = 16; // sizeof(ldouble_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
#if FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
            __float128 tmp = ldouble_t;
            const char* dst = reinterpret_cast<const char*>(&tmp);
#else
            const char* dst = reinterpret_cast<const char*>(&ldouble_t);
#endif // FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
#if FASTCDR_HAVE_FLOAT128 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
            m_currentPosition++ << dst[15];
            m_currentPosition++ << dst[14];
            m_currentPosition++ << dst[13];
            m_currentPosition++ << dst[12];
            m_currentPosition++ << dst[11];
            m_currentPosition++ << dst[10];
            m_currentPosition++ << dst[9];
            m_currentPosition++ << dst[8];
            m_currentPosition++ << dst[7];
            m_currentPosition++ << dst[6];
            m_currentPosition++ << dst[5];
            m_currentPosition++ << dst[4];
            m_currentPosition++ << dst[3];
            m_currentPosition++ << dst[2];
            m_currentPosition++ << dst[1];
            m_currentPosition++ << dst[0];
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8
            // Filled with 0's.
            m_currentPosition++ << static_cast<char>(0);
            m_currentPosition++ << static_cast<char>(0);
            m_currentPosition++ << static_cast<char>(0);
            m_currentPosition++ << static_cast<char>(0);
            m_currentPosition++ << static_cast<char>(0);
            m_currentPosition++ << static_cast<char>(0);
            m_currentPosition++ << static_cast<char>(0);
            m_currentPosition++ << static_cast<char>(0);
            m_currentPosition++ << dst[7];
            m_currentPosition++ << dst[6];
            m_currentPosition++ << dst[5];
            m_currentPosition++ << dst[4];
            m_currentPosition++ << dst[3];
            m_currentPosition++ << dst[2];
            m_currentPosition++ << dst[1];
            m_currentPosition++ << dst[0];
#else
#error unsupported long double type and no __float128 available
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8
#endif // FASTCDR_HAVE_FLOAT128 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
        }
        else
        {
#if FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
            __float128 tmp = ldouble_t;
            m_currentPosition << tmp;
            m_currentPosition += 16;
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8
            m_currentPosition << static_cast<long double>(0);
            m_currentPosition += sizeof(ldouble_t);
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
            m_currentPosition << ldouble_t;
            m_currentPosition += sizeof(ldouble_t);
#else
#error unsupported long double type and no __float128 available
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
#endif // FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serialize(
        const long double ldouble_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serialize(ldouble_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serialize(
        const bool bool_t)
{
    uint8_t value = 0;

    if (((m_lastPosition - m_currentPosition) >= sizeof(uint8_t)) || resize(sizeof(uint8_t)))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(uint8_t);

        if (bool_t)
        {
            value = 1;
        }
        m_currentPosition++ << value;

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serialize(
        const char* string_t)
{
    uint32_t length = 0;

    if (string_t != nullptr)
    {
        length = size_to_uint32(strlen(string_t)) + 1;
    }

    if (length > 0)
    {
        Cdr::state state_before_error(*this);
        serialize(length);

        if (((m_lastPosition - m_currentPosition) >= length) || resize(length))
        {
            // Save last datasize.
            m_lastDataSize = sizeof(uint8_t);

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

Cdr& Cdr::serialize(
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
        Cdr::state state_(*this);
        serialize(size_to_uint32(wstrlen));

        if (((m_lastPosition - m_currentPosition) >= bytesLength) || resize(bytesLength))
        {
            // Save last datasize.
            m_lastDataSize = sizeof(uint32_t);

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

Cdr& Cdr::serialize(
        const char* string_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serialize(string_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serialize(
        const wchar_t* string_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serialize(string_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serializeArray(
        const bool* bool_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*bool_t) * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*bool_t);

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

Cdr& Cdr::serializeArray(
        const char* char_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*char_t) * numElements;

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*char_t);

        m_currentPosition.memcopy(char_t, totalSize);
        m_currentPosition += totalSize;
        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serializeArray(
        const int16_t* short_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*short_t));
    size_t totalSize = sizeof(*short_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*short_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&short_t);
            const char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*short_t))
            {
                m_currentPosition++ << dst[1];
                m_currentPosition++ << dst[0];
            }
        }
        else
        {
            m_currentPosition.memcopy(short_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serializeArray(
        const int16_t* short_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serializeArray(short_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serializeArray(
        const int32_t* long_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*long_t));
    size_t totalSize = sizeof(*long_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*long_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&long_t);
            const char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*long_t))
            {
                m_currentPosition++ << dst[3];
                m_currentPosition++ << dst[2];
                m_currentPosition++ << dst[1];
                m_currentPosition++ << dst[0];
            }
        }
        else
        {
            m_currentPosition.memcopy(long_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serializeArray(
        const int32_t* long_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serializeArray(long_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serializeArray(
        const wchar_t* wchar,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    for (size_t count = 0; count < numElements; ++count)
    {
        serialize(wchar[count]);
    }
    return *this;
}

Cdr& Cdr::serializeArray(
        const wchar_t* wchar,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serializeArray(wchar, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serializeArray(
        const int64_t* longlong_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*longlong_t));
    size_t totalSize = sizeof(*longlong_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*longlong_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&longlong_t);
            const char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*longlong_t))
            {
                m_currentPosition++ << dst[7];
                m_currentPosition++ << dst[6];
                m_currentPosition++ << dst[5];
                m_currentPosition++ << dst[4];
                m_currentPosition++ << dst[3];
                m_currentPosition++ << dst[2];
                m_currentPosition++ << dst[1];
                m_currentPosition++ << dst[0];
            }
        }
        else
        {
            m_currentPosition.memcopy(longlong_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serializeArray(
        const int64_t* longlong_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serializeArray(longlong_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serializeArray(
        const float* float_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*float_t));
    size_t totalSize = sizeof(*float_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*float_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&float_t);
            const char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*float_t))
            {
                m_currentPosition++ << dst[3];
                m_currentPosition++ << dst[2];
                m_currentPosition++ << dst[1];
                m_currentPosition++ << dst[0];
            }
        }
        else
        {
            m_currentPosition.memcopy(float_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serializeArray(
        const float* float_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serializeArray(float_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serializeArray(
        const double* double_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*double_t));
    size_t totalSize = sizeof(*double_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*double_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&double_t);
            const char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*double_t))
            {
                m_currentPosition++ << dst[7];
                m_currentPosition++ << dst[6];
                m_currentPosition++ << dst[5];
                m_currentPosition++ << dst[4];
                m_currentPosition++ << dst[3];
                m_currentPosition++ << dst[2];
                m_currentPosition++ << dst[1];
                m_currentPosition++ << dst[0];
            }
        }
        else
        {
            m_currentPosition.memcopy(double_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serializeArray(
        const double* double_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serializeArray(double_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serializeArray(
        const long double* ldouble_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(ALIGNMENT_LONG_DOUBLE);
    // Fix for Windows ( long doubles only store 8 bytes )
    size_t totalSize = 16 * numElements; // sizeof(*ldouble_t)
    size_t sizeAligned = totalSize + align;

    if (((m_lastPosition - m_currentPosition) >= sizeAligned) || resize(sizeAligned))
    {
        // Save last datasize.
        m_lastDataSize = 16;

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

#if FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
        if (m_swapBytes)
        {
            for (size_t i = 0; i < numElements; ++i, ++ldouble_t)
            {
                __float128 tmp = *ldouble_t;
                const char* dst = reinterpret_cast<const char*>(&tmp);
                m_currentPosition++ << dst[15];
                m_currentPosition++ << dst[14];
                m_currentPosition++ << dst[13];
                m_currentPosition++ << dst[12];
                m_currentPosition++ << dst[11];
                m_currentPosition++ << dst[10];
                m_currentPosition++ << dst[9];
                m_currentPosition++ << dst[8];
                m_currentPosition++ << dst[7];
                m_currentPosition++ << dst[6];
                m_currentPosition++ << dst[5];
                m_currentPosition++ << dst[4];
                m_currentPosition++ << dst[3];
                m_currentPosition++ << dst[2];
                m_currentPosition++ << dst[1];
                m_currentPosition++ << dst[0];
            }
        }
        else
        {
            for (size_t i = 0; i < numElements; ++i, ++ldouble_t)
            {
                __float128 tmp = *ldouble_t;
                m_currentPosition << tmp;
                m_currentPosition += 16;
            }
        }
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
        if (m_swapBytes)
        {
            const char* dst = reinterpret_cast<const char*>(&ldouble_t);
            const char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*ldouble_t))
            {
#if FASTCDR_SIZEOF_LONG_DOUBLE == 16
                m_currentPosition++ << dst[15];
                m_currentPosition++ << dst[14];
                m_currentPosition++ << dst[13];
                m_currentPosition++ << dst[12];
                m_currentPosition++ << dst[11];
                m_currentPosition++ << dst[10];
                m_currentPosition++ << dst[9];
                m_currentPosition++ << dst[8];
#else
                m_currentPosition++ << static_cast<char>(0);
                m_currentPosition++ << static_cast<char>(0);
                m_currentPosition++ << static_cast<char>(0);
                m_currentPosition++ << static_cast<char>(0);
                m_currentPosition++ << static_cast<char>(0);
                m_currentPosition++ << static_cast<char>(0);
                m_currentPosition++ << static_cast<char>(0);
                m_currentPosition++ << static_cast<char>(0);
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 16
                m_currentPosition++ << dst[7];
                m_currentPosition++ << dst[6];
                m_currentPosition++ << dst[5];
                m_currentPosition++ << dst[4];
                m_currentPosition++ << dst[3];
                m_currentPosition++ << dst[2];
                m_currentPosition++ << dst[1];
                m_currentPosition++ << dst[0];
            }
        }
        else
        {
#if FASTCDR_SIZEOF_LONG_DOUBLE == 16
            m_currentPosition.memcopy(ldouble_t, totalSize);
            m_currentPosition += totalSize;
#else
            for (size_t i = 0; i < numElements; ++i)
            {
                m_currentPosition << static_cast<long double>(0);
                m_currentPosition += 8;
                m_currentPosition << ldouble_t[i];
                m_currentPosition += 8;
            }
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 16
        }
#else
#error unsupported long double type and no __float128 available
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
#endif // FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::serializeArray(
        const long double* ldouble_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        serializeArray(ldouble_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserialize(
        char& char_t)
{
    if ((m_lastPosition - m_currentPosition) >= sizeof(char_t))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(char_t);

        m_currentPosition++ >> char_t;
        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserialize(
        int16_t& short_t)
{
    size_t align = alignment(sizeof(short_t));
    size_t sizeAligned = sizeof(short_t) + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(short_t);

        // Align
        makeAlign(align);

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(&short_t);

            m_currentPosition++ >> dst[1];
            m_currentPosition++ >> dst[0];
        }
        else
        {
            m_currentPosition >> short_t;
            m_currentPosition += sizeof(short_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserialize(
        int16_t& short_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserialize(short_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserialize(
        int32_t& long_t)
{
    size_t align = alignment(sizeof(long_t));
    size_t sizeAligned = sizeof(long_t) + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(long_t);

        // Align
        makeAlign(align);

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(&long_t);

            m_currentPosition++ >> dst[3];
            m_currentPosition++ >> dst[2];
            m_currentPosition++ >> dst[1];
            m_currentPosition++ >> dst[0];
        }
        else
        {
            m_currentPosition >> long_t;
            m_currentPosition += sizeof(long_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserialize(
        int32_t& long_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserialize(long_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserialize(
        int64_t& longlong_t)
{
    size_t align = alignment(sizeof(longlong_t));
    size_t sizeAligned = sizeof(longlong_t) + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(longlong_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(&longlong_t);

            m_currentPosition++ >> dst[7];
            m_currentPosition++ >> dst[6];
            m_currentPosition++ >> dst[5];
            m_currentPosition++ >> dst[4];
            m_currentPosition++ >> dst[3];
            m_currentPosition++ >> dst[2];
            m_currentPosition++ >> dst[1];
            m_currentPosition++ >> dst[0];
        }
        else
        {
            m_currentPosition >> longlong_t;
            m_currentPosition += sizeof(longlong_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserialize(
        int64_t& longlong_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserialize(longlong_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserialize(
        float& float_t)
{
    size_t align = alignment(sizeof(float_t));
    size_t sizeAligned = sizeof(float_t) + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(float_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(&float_t);

            m_currentPosition++ >> dst[3];
            m_currentPosition++ >> dst[2];
            m_currentPosition++ >> dst[1];
            m_currentPosition++ >> dst[0];
        }
        else
        {
            m_currentPosition >> float_t;
            m_currentPosition += sizeof(float_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserialize(
        float& float_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserialize(float_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserialize(
        double& double_t)
{
    size_t align = alignment(sizeof(double_t));
    size_t sizeAligned = sizeof(double_t) + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(double_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(&double_t);

            m_currentPosition++ >> dst[7];
            m_currentPosition++ >> dst[6];
            m_currentPosition++ >> dst[5];
            m_currentPosition++ >> dst[4];
            m_currentPosition++ >> dst[3];
            m_currentPosition++ >> dst[2];
            m_currentPosition++ >> dst[1];
            m_currentPosition++ >> dst[0];
        }
        else
        {
            m_currentPosition >> double_t;
            m_currentPosition += sizeof(double_t);
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserialize(
        double& double_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserialize(double_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserialize(
        long double& ldouble_t)
{
    size_t align = alignment(ALIGNMENT_LONG_DOUBLE);
    size_t sizeAligned = sizeof(ldouble_t) + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = 16; // sizeof(ldouble_t);

        // Align.
        makeAlign(align);

        if (m_swapBytes)
        {
#if FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
            __float128 tmp = ldouble_t;
            char* dst = reinterpret_cast<char*>(&tmp);
#else
            char* dst = reinterpret_cast<char*>(&ldouble_t);
#endif // FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
#if FASTCDR_HAVE_FLOAT128 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
            m_currentPosition++ >> dst[15];
            m_currentPosition++ >> dst[14];
            m_currentPosition++ >> dst[13];
            m_currentPosition++ >> dst[12];
            m_currentPosition++ >> dst[11];
            m_currentPosition++ >> dst[10];
            m_currentPosition++ >> dst[9];
            m_currentPosition++ >> dst[8];
            m_currentPosition++ >> dst[7];
            m_currentPosition++ >> dst[6];
            m_currentPosition++ >> dst[5];
            m_currentPosition++ >> dst[4];
            m_currentPosition++ >> dst[3];
            m_currentPosition++ >> dst[2];
            m_currentPosition++ >> dst[1];
            m_currentPosition++ >> dst[0];
#if FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
            ldouble_t = static_cast<long double>(tmp);
#endif // FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8
            m_currentPosition += 8;
            m_currentPosition++ >> dst[7];
            m_currentPosition++ >> dst[6];
            m_currentPosition++ >> dst[5];
            m_currentPosition++ >> dst[4];
            m_currentPosition++ >> dst[3];
            m_currentPosition++ >> dst[2];
            m_currentPosition++ >> dst[1];
            m_currentPosition++ >> dst[0];
#else
#error unsupported long double type and no __float128 available
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8
#endif // FASTCDR_HAVE_FLOAT128 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
        }
        else
        {
#if FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
            __float128 tmp;
            m_currentPosition >> tmp;
            m_currentPosition += 16;
            ldouble_t = static_cast<long double>(tmp);
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8
            m_currentPosition += 8;
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8
            m_currentPosition >> ldouble_t;
            m_currentPosition += sizeof(ldouble_t);
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
#endif // FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserialize(
        long double& ldouble_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserialize(ldouble_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserialize(
        bool& bool_t)
{
    uint8_t value = 0;

    if ((m_lastPosition - m_currentPosition) >= sizeof(uint8_t))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(uint8_t);

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

        throw BadParamException("Unexpected byte value in Cdr::deserialize(bool), expected 0 or 1");
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserialize(
        char*& string_t)
{
    uint32_t length = 0;
    Cdr::state state_before_error(*this);

    deserialize(length);

    if (length == 0)
    {
        string_t = NULL;
        return *this;
    }
    else if ((m_lastPosition - m_currentPosition) >= length)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(uint8_t);

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

Cdr& Cdr::deserialize(
        wchar_t*& string_t)
{
    uint32_t length = 0;
    Cdr::state state_before_error(*this);

    deserialize(length);

    if (length == 0)
    {
        string_t = NULL;
        return *this;
    }
    else if ((m_lastPosition - m_currentPosition) >= length)
    {
        // Save last datasize.
        m_lastDataSize = 4;
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

Cdr& Cdr::deserialize(
        char*& string_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserialize(string_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserialize(
        wchar_t*& string_t,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserialize(string_t);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

const char* Cdr::readString(
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
        // Save last datasize.
        m_lastDataSize = sizeof(uint8_t);

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

const std::wstring Cdr::readWString(
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
        // Save last datasize.
        m_lastDataSize = sizeof(uint32_t);

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

Cdr& Cdr::deserializeArray(
        bool* bool_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*bool_t) * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*bool_t);

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

Cdr& Cdr::deserializeArray(
        char* char_t,
        size_t numElements)
{
    size_t totalSize = sizeof(*char_t) * numElements;

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*char_t);

        m_currentPosition.rmemcopy(char_t, totalSize);
        m_currentPosition += totalSize;
        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserializeArray(
        int16_t* short_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*short_t));
    size_t totalSize = sizeof(*short_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*short_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(short_t);
            char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*short_t))
            {
                m_currentPosition++ >> dst[1];
                m_currentPosition++ >> dst[0];
            }
        }
        else
        {
            m_currentPosition.rmemcopy(short_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserializeArray(
        int16_t* short_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserializeArray(short_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserializeArray(
        int32_t* long_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*long_t));
    size_t totalSize = sizeof(*long_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*long_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(long_t);
            char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*long_t))
            {
                m_currentPosition++ >> dst[3];
                m_currentPosition++ >> dst[2];
                m_currentPosition++ >> dst[1];
                m_currentPosition++ >> dst[0];
            }
        }
        else
        {
            m_currentPosition.rmemcopy(long_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserializeArray(
        int32_t* long_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserializeArray(long_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserializeArray(
        wchar_t* wchar,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    uint32_t value;
    for (size_t count = 0; count < numElements; ++count)
    {
        deserialize(value);
        wchar[count] = static_cast<wchar_t>(value);
    }
    return *this;
}

Cdr& Cdr::deserializeArray(
        wchar_t* wchar,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserializeArray(wchar, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserializeArray(
        int64_t* longlong_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*longlong_t));
    size_t totalSize = sizeof(*longlong_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*longlong_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(longlong_t);
            char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*longlong_t))
            {
                m_currentPosition++ >> dst[7];
                m_currentPosition++ >> dst[6];
                m_currentPosition++ >> dst[5];
                m_currentPosition++ >> dst[4];
                m_currentPosition++ >> dst[3];
                m_currentPosition++ >> dst[2];
                m_currentPosition++ >> dst[1];
                m_currentPosition++ >> dst[0];
            }
        }
        else
        {
            m_currentPosition.rmemcopy(longlong_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserializeArray(
        int64_t* longlong_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserializeArray(longlong_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserializeArray(
        float* float_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*float_t));
    size_t totalSize = sizeof(*float_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*float_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(float_t);
            char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*float_t))
            {
                m_currentPosition++ >> dst[3];
                m_currentPosition++ >> dst[2];
                m_currentPosition++ >> dst[1];
                m_currentPosition++ >> dst[0];
            }
        }
        else
        {
            m_currentPosition.rmemcopy(float_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserializeArray(
        float* float_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserializeArray(float_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserializeArray(
        double* double_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(sizeof(*double_t));
    size_t totalSize = sizeof(*double_t) * numElements;
    size_t sizeAligned = totalSize + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = sizeof(*double_t);

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(double_t);
            char* end = dst + totalSize;

            for (; dst < end; dst += sizeof(*double_t))
            {
                m_currentPosition++ >> dst[7];
                m_currentPosition++ >> dst[6];
                m_currentPosition++ >> dst[5];
                m_currentPosition++ >> dst[4];
                m_currentPosition++ >> dst[3];
                m_currentPosition++ >> dst[2];
                m_currentPosition++ >> dst[1];
                m_currentPosition++ >> dst[0];
            }
        }
        else
        {
            m_currentPosition.rmemcopy(double_t, totalSize);
            m_currentPosition += totalSize;
        }

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserializeArray(
        double* double_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserializeArray(double_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::deserializeArray(
        long double* ldouble_t,
        size_t numElements)
{
    if (numElements == 0)
    {
        return *this;
    }

    size_t align = alignment(ALIGNMENT_LONG_DOUBLE);
    // Fix for Windows ( long doubles only store 8 bytes )
    size_t totalSize = 16 * numElements;
    size_t sizeAligned = totalSize + align;

    if ((m_lastPosition - m_currentPosition) >= sizeAligned)
    {
        // Save last datasize.
        m_lastDataSize = 16;

        // Align if there are any elements
        if (numElements)
        {
            makeAlign(align);
        }

#if FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16
        if (m_swapBytes)
        {
            for (size_t i = 0; i < numElements; ++i)
            {
                __float128 tmp;
                char* dst = reinterpret_cast<char*>(&tmp);
                m_currentPosition++ >> dst[15];
                m_currentPosition++ >> dst[14];
                m_currentPosition++ >> dst[13];
                m_currentPosition++ >> dst[12];
                m_currentPosition++ >> dst[11];
                m_currentPosition++ >> dst[10];
                m_currentPosition++ >> dst[9];
                m_currentPosition++ >> dst[8];
                m_currentPosition++ >> dst[7];
                m_currentPosition++ >> dst[6];
                m_currentPosition++ >> dst[5];
                m_currentPosition++ >> dst[4];
                m_currentPosition++ >> dst[3];
                m_currentPosition++ >> dst[2];
                m_currentPosition++ >> dst[1];
                m_currentPosition++ >> dst[0];
                ldouble_t[i] = static_cast<long double>(tmp);
            }
        }
        else
        {
            for (size_t i = 0; i < numElements; ++i)
            {
                __float128 tmp;
                m_currentPosition >> tmp;
                m_currentPosition += 16;
                ldouble_t[i] = static_cast<long double>(tmp);
            }
        }
#else
#if FASTCDR_SIZEOF_LONG_DOUBLE == 8 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
        if (m_swapBytes)
        {
            char* dst = reinterpret_cast<char*>(ldouble_t);
            char* end = dst + numElements * sizeof(*ldouble_t);

            for (; dst < end; dst += sizeof(*ldouble_t))
            {
#if FASTCDR_SIZEOF_LONG_DOUBLE == 16
                m_currentPosition++ >> dst[15];
                m_currentPosition++ >> dst[14];
                m_currentPosition++ >> dst[13];
                m_currentPosition++ >> dst[12];
                m_currentPosition++ >> dst[11];
                m_currentPosition++ >> dst[10];
                m_currentPosition++ >> dst[9];
                m_currentPosition++ >> dst[8];
#else
                m_currentPosition += 8;
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 16
                m_currentPosition++ >> dst[7];
                m_currentPosition++ >> dst[6];
                m_currentPosition++ >> dst[5];
                m_currentPosition++ >> dst[4];
                m_currentPosition++ >> dst[3];
                m_currentPosition++ >> dst[2];
                m_currentPosition++ >> dst[1];
                m_currentPosition++ >> dst[0];
            }
        }
        else
        {
#if FASTCDR_SIZEOF_LONG_DOUBLE == 16
            m_currentPosition.rmemcopy(ldouble_t, totalSize);
            m_currentPosition += totalSize;
#else
            for (size_t i = 0; i < numElements; ++i)
            {
                m_currentPosition += 8; // ignore first 8 bytes
                m_currentPosition >> ldouble_t[i];
                m_currentPosition += 8;
            }
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 16
        }
#endif // FASTCDR_SIZEOF_LONG_DOUBLE == 8 || FASTCDR_SIZEOF_LONG_DOUBLE == 16
#endif // FASTCDR_HAVE_FLOAT128 && FASTCDR_SIZEOF_LONG_DOUBLE < 16

        return *this;
    }

    throw NotEnoughMemoryException(NotEnoughMemoryException::NOT_ENOUGH_MEMORY_MESSAGE_DEFAULT);
}

Cdr& Cdr::deserializeArray(
        long double* ldouble_t,
        size_t numElements,
        Endianness endianness)
{
    bool auxSwap = m_swapBytes;
    m_swapBytes = (m_swapBytes && (m_endianness == endianness)) || (!m_swapBytes && (m_endianness != endianness));

    try
    {
        deserializeArray(ldouble_t, numElements);
        m_swapBytes = auxSwap;
    }
    catch (Exception& ex)
    {
        m_swapBytes = auxSwap;
        ex.raise();
    }

    return *this;
}

Cdr& Cdr::serializeBoolSequence(
        const std::vector<bool>& vector_t)
{
    state state_before_error(*this);

    *this << static_cast<int32_t>(vector_t.size());

    size_t totalSize = vector_t.size() * sizeof(bool);

    if (((m_lastPosition - m_currentPosition) >= totalSize) || resize(totalSize))
    {
        // Save last datasize.
        m_lastDataSize = sizeof(bool);

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

Cdr& Cdr::deserializeBoolSequence(
        std::vector<bool>& vector_t)
{
    uint32_t seqLength = 0;
    state state_before_error(*this);

    *this >> seqLength;

    size_t totalSize = seqLength * sizeof(bool);

    if ((m_lastPosition - m_currentPosition) >= totalSize)
    {
        vector_t.resize(seqLength);
        // Save last datasize.
        m_lastDataSize = sizeof(bool);

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
            else
            {
                throw BadParamException("Unexpected byte value in Cdr::deserializeBoolSequence, expected 0 or 1");
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

Cdr& Cdr::deserializeStringSequence(
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

Cdr& Cdr::deserializeWStringSequence(
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
