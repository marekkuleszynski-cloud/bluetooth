#include "BluetoothManager.h"

void BluetoothManager::begin()
{
    m_sink.start("ESP32 Audio Receiver");
}

bool BluetoothManager::isConnected() const
{
    return false;
}