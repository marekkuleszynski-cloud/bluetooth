#include "AudioManager.h"

AudioManager::AudioManager(AudioBuffer& buffer, I2SManager& i2s)
    : m_buffer(buffer),
      m_i2s(i2s)
{
}

void AudioManager::begin()
{
    m_volume = 100;
}

void AudioManager::process()
{
    uint8_t data[256];

    size_t bytesRead = m_buffer.read(data, sizeof(data));

    if (bytesRead > 0)
    {
        size_t bytesWritten = m_i2s.write(data, bytesRead);

        static uint32_t counter = 0;
        counter++;

        if (counter % 100 == 0)
        {
            Serial.printf(
                "AudioManager: read=%u, i2s_written=%u\n",
                bytesRead,
                bytesWritten
            );
        }
    }
}

void AudioManager::setVolume(uint8_t volume)
{
    if (volume > 100)
    {
        volume = 100;
    }

    m_volume = volume;
}

uint8_t AudioManager::getVolume() const
{
    return m_volume;
}

void AudioManager::playConnectedTone()
{
    // TODO
}

void AudioManager::playDisconnectedTone()
{
    // TODO
}

void AudioManager::playSearchingTone()
{
    // TODO
}