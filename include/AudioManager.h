#pragma once

#include <stdint.h>

class AudioManager
{
public:
    void begin();

    void setVolume(uint8_t volume);
    uint8_t getVolume() const;

    void playConnectedTone();
    void playDisconnectedTone();
    void playSearchingTone();

private:
    uint8_t m_volume = 100;
};