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