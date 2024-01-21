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

#ifndef _FASTCDR_CDRBUFFER_H_
#define _FASTCDR_CDRBUFFER_H_

#include "fastcdr_dll.h"
#include <stdint.h>
#include <cstdio>
#include <string.h>
#include <cstddef>
#include <utility>

inline uint32_t size_to_uint32(
        size_t val)
{
  #if defined(_WIN32) || !defined(FASTCDR_ARM32)
    // On 64 bit platforms and all Windows architectures (because of C4267), explicitly cast.
    return static_cast<uint32_t>(val);
  #else
    // Skip useless cast on 32-bit builds.
    return val;
  #endif // if defined(_WIN32) || !defined(FASTCDR_ARM32)
}

namespace eprosima {
namespace fastcdr {
/*!
 * @brief This class implements the iterator used to go through a FastBuffer.
 */
class Cdr_DllAPI _FastBuffer_iterator
{
public:

    /*!
     * @brief Default constructor.
     * The iterator points any position.
     */
    _FastBuffer_iterator() = default;

    /*!
     * @brief Constructor.
     * The iterator points to the indicated position.
     * @param buffer Pointer to the raw buffer.
     * @param index Position of the raw buffer where the iterator will point.
     */
    explicit _FastBuffer_iterator(
            char* buffer,
            size_t index)
        : buffer_(buffer)
        , current_position_(&buffer_[index])
    {
    }

    /*!
     * @brief This operator changes the iterator's raw buffer.
     * This operator makes the iterator point to the same position but in another raw buffer.
     * The new raw buffer is the same than the source iterator's.
     * @param iterator The source iterator. The iterator will use the source iterator's raw buffer after this operation.
     */
    inline
    void operator <<(
            const _FastBuffer_iterator& iterator)
    {
        ptrdiff_t diff = current_position_ - buffer_;
        buffer_ = iterator.buffer_;
        current_position_ = buffer_ + diff;
    }

    /*!
     * @brief This operator changes the position where the iterator points.
     * This operator takes the index of the source iterator, but the iterator continues using its raw buffer.
     * @param iterator The source iterator. The iterator will use the source's iterator index to point to its own raw buffer.
     */
    inline
    void operator >>(
            const _FastBuffer_iterator& iterator)
    {
        ptrdiff_t diff = iterator.current_position_ - iterator.buffer_;
        current_position_ = buffer_ + diff;
    }

    /*!
     * @brief This operator copies a data in the raw buffer.
     * The copy uses the size of the data type.
     * @param data Data to be copied. Cannot be NULL.
     */
    template<typename _T>
    inline
    void operator <<(
            const _T& data)
    {
        memcpy(current_position_, &data, sizeof(_T));
    }

    /*!
     * @brief This operator copies data from the raw buffer to a variable.
     * The copy uses the size of the data type.
     * @param data Data to be filled.
     */
    template<typename _T>
    inline
    void operator >>(
            _T& data)
    {
        memcpy(&data, current_position_, sizeof(_T));
    }

    /*!
     * @brief This function copies a buffer into the raw buffer.
     * @param src The source buffer.
     * @param size The number of bytes to be copied.
     */
    inline
    void memcopy(
            const void* src,
            const size_t size)
    {
        if (size > 0)
        {
            memcpy(current_position_, src, size);
        }
    }

    /*!
     * @brief This function copies from the raw buffer to a external buffer.
     * @param dst The destination buffer.
     * @param size The size of bytes to be copied.
     */
    inline
    void rmemcopy(
            void* dst,
            const size_t size)
    {
        if (size > 0)
        {
            memcpy(dst, current_position_, size);
        }
    }

    /*!
     * @brief This function increments the position where the iterator points.
     * @param num_bytes Number of bytes the iterator moves the position.
     */
    inline
    void operator +=(
            size_t num_bytes)
    {
        current_position_ += num_bytes;
    }

    inline
    void operator -=(
            size_t num_bytes)
    {
        current_position_ -= num_bytes;
    }

    /*!
     * @brief This operator returns the subtraction of the current interator's position and the source iterator's position.
     * @param it Source iterator whose position is subtracted to the current iterator's position.
     * @return The result of subtract the current iterator's position and the source iterator's position.
     */
    inline
    size_t operator -(
            const _FastBuffer_iterator& it) const
    {
        return static_cast<size_t>(current_position_ - it.current_position_);
    }

    /*!
     * @brief This function increments the iterator in one the position.
     * @return The current iterator.
     */
    inline
    _FastBuffer_iterator operator ++()
    {
        ++current_position_;
        return *this;
    }

    /*!
     * @brief This function increments the iterator in one the position.
     * @return The current iterator.
     */
    inline
    _FastBuffer_iterator operator ++(
            int)
    {
        _FastBuffer_iterator tmp = *this;
        ++*this;
        return tmp;
    }

    /*!
     * @brief This function returns the current position in the raw buffer.
     * @return The current position in the raw buffer.
     */
    inline
    char* operator &()
    {
        return current_position_;
    }

    bool operator ==(
            const _FastBuffer_iterator& other_iterator) const
    {
        return other_iterator.current_position_ == current_position_;
    }

    bool operator !=(
            const _FastBuffer_iterator& other_iterator) const
    {
        return !(other_iterator == *this);
    }

private:

    //! Pointer to the raw buffer.
    char* buffer_ {nullptr};

    //! Current position in the raw buffer.
    char* current_position_ {nullptr};
};

/*!
 * @brief This class represents a stream of bytes that contains (or will contain)
 * serialized data. This class is used by the serializers to serialize
 * or deserialize using their representation.
 * @ingroup FASTCDRAPIREFERENCE
 */
class Cdr_DllAPI FastBuffer
{
public:

    typedef _FastBuffer_iterator iterator;

    /*!
     * @brief This constructor creates an internal stream and assigns it to the eprosima::fastcdr::FastBuffers object.
     * The user can obtain this internal stream using the function eprosima::fastcdr::FastBuffers::getBuffer(). Be careful because this internal stream
     * is deleted in the destruction of the eprosima::fastcdr::FastBuffers object.
     */
    FastBuffer() = default;

    /*!
     * @brief This constructor assigns the user's stream of bytes to the eprosima::fastcdr::FastBuffers object.
     * The user's stream will be used to serialize.
     *
     * @param buffer The user's buffer that will be used. This buffer is not deallocated in the object's destruction. Cannot be NULL.
     * @param bufferSize The length of user's buffer.
     */
    FastBuffer(
            char* const buffer,
            const size_t bufferSize);

    //! Move constructor
    FastBuffer(
            FastBuffer&& fbuffer)
        : buffer_(nullptr)
        , size_(0)
        , m_internalBuffer(true)
    {
        std::swap(buffer_, fbuffer.buffer_);
        std::swap(size_, fbuffer.size_);
        std::swap(m_internalBuffer, fbuffer.m_internalBuffer);
    }

    //! Move assignment
    FastBuffer& operator =(
            FastBuffer&& fbuffer)
    {
        std::swap(buffer_, fbuffer.buffer_);
        std::swap(size_, fbuffer.size_);
        std::swap(m_internalBuffer, fbuffer.m_internalBuffer);
        return *this;
    }

    /*!
     * @brief Default destructor.
     */
    virtual ~FastBuffer();

    /*!
     * @brief This function returns the stream that the eprosima::fastcdr::FastBuffers uses to serialize data.
     * @return The stream used by eprosima::fastcdr::FastBuffers to serialize data.
     */
    inline char* getBuffer() const
    {
        return buffer_;
    }

    /*!
     * @brief This function returns the size of the allocated memory of the stream that the eprosima::fastcdr::FastBuffers uses to serialize data.
     * @return The size of the allocated memory of the stream used by the eprosima::fastcdr::FastBuffers to serialize data.
     */
    inline size_t getBufferSize() const
    {
        return size_;
    }

    /*!
     * @brief This function returns a iterator that points to the begining of the stream.
     * @return The new iterator.
     */
    inline
    iterator begin()
    {
        return (iterator(buffer_, 0));
    }

    /*!
     * @brief This function returns a iterator that points to the end of the stream.
     * @return The new iterator.
     */
    inline
    iterator end()
    {
        return (iterator(buffer_, size_));
    }

    /*!
     * @brief This function reserves memory for the internal raw buffer. It will only do so if the buffer is not yet allocated and is not externally set.
     * @param size The size of the memory to be allocated.
     * @return True if the allocation suceeded. False if the raw buffer was set externally or is already allocated.
     */
    bool reserve(
            size_t size);

    /*!
     * @brief This function resizes the raw buffer. It will call the user's defined function for this purpose.
     * @param min_size_inc The minimun growth expected of the current raw buffer.
     * @return True if the operation works. False if it does not.
     */
    bool resize(
            size_t min_size_inc);

private:

    FastBuffer(
            const FastBuffer&) = delete;

    FastBuffer& operator =(
            const FastBuffer&) = delete;

    //! @brief Pointer to the stream of bytes that contains the serialized data.
    char* buffer_ { nullptr };

    //! @brief The total size of the user's buffer.
    size_t size_ { 0 };

    //! @brief This variable indicates if the managed buffer is internal or is from the user.
    bool m_internalBuffer { true };
};
}     //namespace fastcdr
} //namespace eprosima

#endif // _FASTCDR_FASTCDRBUFFER_H_
