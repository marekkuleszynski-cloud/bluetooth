#pragma once

#include <BluetoothA2DPSink.h>

class BluetoothManager
{
public:
    void begin();

    bool isConnected() const;

private:
    BluetoothA2DPSink m_sink;
};