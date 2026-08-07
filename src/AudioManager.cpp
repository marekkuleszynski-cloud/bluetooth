#include "AudioManager.h"

void AudioManager::begin()
{
    m_volume = 100;
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
    // TODO: Generate and play connected tone
}

void AudioManager::playDisconnectedTone()
{
    // TODO: Generate and play disconnected tone
}

void AudioManager::playSearchingTone()
{
    // TODO: Generate and play searching tone
}