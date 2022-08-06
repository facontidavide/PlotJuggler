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

FastBuffer::FastBuffer()
    : m_buffer(nullptr)
    , m_bufferSize(0)
    , m_internalBuffer(true)
{
}

FastBuffer::FastBuffer(
        char* const buffer,
        const size_t bufferSize)
    : m_buffer(buffer)
    , m_bufferSize(bufferSize)
    , m_internalBuffer(false)
{
}

FastBuffer::~FastBuffer()
{
    if (m_internalBuffer && m_buffer != nullptr)
    {
        free(m_buffer);
    }
}

bool FastBuffer::reserve(
        size_t size)
{
    if (m_internalBuffer && m_buffer == NULL)
    {
        m_buffer = reinterpret_cast<char*>(malloc(size));
        if (m_buffer)
        {
            m_bufferSize = size;
            return true;
        }
    }
    return false;
}

bool FastBuffer::resize(
        size_t minSizeInc)
{
    size_t incBufferSize = BUFFER_START_LENGTH;

    if (m_internalBuffer)
    {
        if (minSizeInc > BUFFER_START_LENGTH)
        {
            incBufferSize = minSizeInc;
        }

        if (m_buffer == NULL)
        {
            m_bufferSize = incBufferSize;

            m_buffer = reinterpret_cast<char*>(malloc(m_bufferSize));

            if (m_buffer != NULL)
            {
                return true;
            }
        }
        else
        {
            m_bufferSize += incBufferSize;

            m_buffer = reinterpret_cast<char*>(realloc(m_buffer, m_bufferSize));

            if (m_buffer != NULL)
            {
                return true;
            }
        }
    }

    return false;
}
