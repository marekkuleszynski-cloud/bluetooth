#pragma once

#include <stdint.h>
#include "AudioBuffer.h"
#include "I2SManager.h"

class AudioManager
{
public:
    AudioManager(AudioBuffer& buffer, I2SManager& i2s);

    void begin();
    void process();

    void setVolume(uint8_t volume);
    uint8_t getVolume() const;

    void playConnectedTone();
    void playDisconnectedTone();
    void playSearchingTone();

private:
    AudioBuffer& m_buffer;
    I2SManager& m_i2s;

    uint8_t m_volume = 100;
};