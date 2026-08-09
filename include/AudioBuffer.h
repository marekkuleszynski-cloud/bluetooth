#pragma once

#include <stdint.h>
#include <stddef.h>

class AudioBuffer
{
public:
    AudioBuffer();

    bool begin(size_t size);

    size_t write(const uint8_t* data, size_t length);
    size_t read(uint8_t* data, size_t length);

    size_t available() const;
    size_t freeSpace() const;

private:
    uint8_t* m_buffer = nullptr;

    size_t m_size = 0;
    size_t m_writeIndex = 0;
    size_t m_readIndex = 0;
};