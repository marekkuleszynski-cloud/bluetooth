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
    m_available = 0;

    return true;
}

size_t AudioBuffer::write(const uint8_t *data, size_t length)
{
    if (data == nullptr || length == 0)
    {
        return 0;
    }

    size_t space = m_size - m_available;

    if (space == 0)
    {
        return 0;
    }

    size_t bytesToWrite = length;

    if (bytesToWrite > space)
    {
        bytesToWrite = space;
    }

    for (size_t i = 0; i < bytesToWrite; i++)
    {
        m_buffer[m_writeIndex] = data[i];

        m_writeIndex++;

        if (m_writeIndex >= m_size)
        {
            m_writeIndex = 0;
        }
    }

    m_available += bytesToWrite;

    return bytesToWrite;
}

size_t AudioBuffer::read(uint8_t *data, size_t length)
{
    if (data == nullptr || length == 0)
    {
        return 0;
    }

    if (m_available == 0)
    {
        return 0;
    }

    size_t bytesToRead = length;

    if (bytesToRead > m_available)
    {
        bytesToRead = m_available;
    }

    for (size_t i = 0; i < bytesToRead; i++)
    {
        data[i] = m_buffer[m_readIndex];

        m_readIndex++;

        if (m_readIndex >= m_size)
        {
            m_readIndex = 0;
        }
    }

    m_available -= bytesToRead;

    return bytesToRead;
}

size_t AudioBuffer::available() const
{
    return m_available;
}

size_t AudioBuffer::freeSpace() const
{
    return m_size - m_available;
}