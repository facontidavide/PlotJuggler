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

#include <fastcdr/FastBuffer.h>

#if !__APPLE__ && !__FreeBSD__ && !__VXWORKS__
#include <malloc.h>
#else
#include <stdlib.h>
#endif // if !__APPLE__ && !__FreeBSD__ && !__VXWORKS__

#define BUFFER_START_LENGTH 200

using namespace eprosima::fastcdr;

FastBuffer::FastBuffer(
        char* const buffer,
        const size_t bufferSize)
    : buffer_(buffer)
    , size_(bufferSize)
    , m_internalBuffer(false)
{
}

FastBuffer::~FastBuffer()
{
    if (m_internalBuffer && buffer_ != nullptr)
    {
        free(buffer_);
    }
}

bool FastBuffer::reserve(
        size_t size)
{
    if (m_internalBuffer && buffer_ == NULL)
    {
        buffer_ = reinterpret_cast<char*>(malloc(size));
        if (buffer_)
        {
            size_ = size;
            return true;
        }
    }
    return false;
}

bool FastBuffer::resize(
        size_t min_size_inc)
{
    size_t incBufferSize = BUFFER_START_LENGTH;

    if (m_internalBuffer)
    {
        if (min_size_inc > BUFFER_START_LENGTH)
        {
            incBufferSize = min_size_inc;
        }

        if (buffer_ == NULL)
        {
            size_ = incBufferSize;

            buffer_ = reinterpret_cast<char*>(malloc(size_));

            if (buffer_ != NULL)
            {
                return true;
            }
        }
        else
        {
            size_ += incBufferSize;

            buffer_ = reinterpret_cast<char*>(realloc(buffer_, size_));

            if (buffer_ != NULL)
            {
                return true;
            }
        }
    }

    return false;
}
