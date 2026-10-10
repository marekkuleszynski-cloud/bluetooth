#include <Arduino.h>
#include <BluetoothA2DPSink.h>
#include "BluetoothManager.h"
#include "AudioEngine.h"
#include "AppState.h"

BluetoothA2DPSink a2dpSink(i2s);
void btConnectionStateChanged(
    esp_a2d_connection_state_t state,
    void*
)
{
    if (state == ESP_A2D_CONNECTION_STATE_CONNECTED)
    {
        btConnected = true;
        btConnectionChanged = true;
    }
    else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
    {
        btConnected = false;
        btConnectionChanged = true;
    }
}
void beginBluetooth()
{
    a2dpSink.set_on_connection_state_changed(btConnectionStateChanged);
    a2dpSink.set_stream_reader(audioCallback,false);
    a2dpSink.start("Marocco's BT Audio Receiver");
    Serial.println("Bluetooth A2DP + DSP EQ started");
    Serial.println("Waiting for BT audio...");
}
