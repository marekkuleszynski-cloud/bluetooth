#pragma once

#include <BluetoothA2DPSink.h>
#include "AudioBuffer.h"

class BluetoothManager
{
public:
    BluetoothManager(AudioBuffer& buffer);

    void begin();
    bool isConnected();

private:
    AudioBuffer& m_buffer;
    BluetoothA2DPSink m_sink;

    static BluetoothManager* s_instance;

    static void audioDataCallback(
        const uint8_t* data,
        uint32_t length);
};