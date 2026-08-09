#include <Arduino.h>
#include "BluetoothManager.h"

BluetoothManager* BluetoothManager::s_instance = nullptr;

BluetoothManager::BluetoothManager(AudioBuffer& buffer)
    : m_buffer(buffer)
{
    s_instance = this;
}

void BluetoothManager::begin()
{
    m_sink.set_stream_reader(audioDataCallback, false);

    m_sink.start("ESP32 Audio Receiver");
}

bool BluetoothManager::isConnected()
{
    return m_sink.is_connected();
}

void BluetoothManager::audioDataCallback(
    const uint8_t* data,
    uint32_t length)
{
    if (s_instance == nullptr)
    {
        return;
    }

    size_t written = s_instance->m_buffer.write(data, length);

    Serial.printf(
        "Received: %lu | Written: %u | Buffer: %u\n",
        length,
        written,
        s_instance->m_buffer.available()
    );
}