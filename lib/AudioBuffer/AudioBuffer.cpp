#include "AudioBuffer.h"

AudioBuffer::AudioBuffer()
{
}

bool AudioBuffer::begin(size_t size)
{
    if (size == 0)
    {
        return false;
    }

    m_buffer = new uint8_t[size];

    if (m_buffer == nullptr)
    {
        return false;
    }

    m_size = size;
    m_writeIndex = 0;
    m_readIndex = 0;

    return true;
}

size_t AudioBuffer::write(const uint8_t* data, size_t length)
{
    if (data == nullptr || length == 0)
    {
        return 0;
    }

    size_t bytesWritten = 0;

    while (bytesWritten < length)
    {
        size_t nextWriteIndex = m_writeIndex + 1;

        if (nextWriteIndex >= m_size)
        {
            nextWriteIndex = 0;
        }

        // Buffer full
        if (nextWriteIndex == m_readIndex)
        {
            break;
        }

        m_buffer[m_writeIndex] = data[bytesWritten];

        m_writeIndex = nextWriteIndex;

        bytesWritten++;
    }

    return bytesWritten;
}

size_t AudioBuffer::read(uint8_t* data, size_t length)
{
    if (data == nullptr || length == 0)
    {
        return 0;
    }

    size_t bytesRead = 0;

    while (bytesRead < length)
    {
        // Buffer empty
        if (m_readIndex == m_writeIndex)
        {
            break;
        }

        data[bytesRead] = m_buffer[m_readIndex];

        m_readIndex++;

        if (m_readIndex >= m_size)
        {
            m_readIndex = 0;
        }

        bytesRead++;
    }

    return bytesRead;
}

size_t AudioBuffer::available() const
{
    if (m_writeIndex >= m_readIndex)
    {
        return m_writeIndex - m_readIndex;
    }

    return m_size - m_readIndex + m_writeIndex;
}

size_t AudioBuffer::freeSpace() const
{
    return (m_size - 1) - available();
}