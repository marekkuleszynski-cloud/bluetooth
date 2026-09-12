#include <Arduino.h>
#include <ESP_I2S.h>
#include <BluetoothA2DPSink.h>

I2SClass i2s;
BluetoothA2DPSink a2dpSink(i2s);

void setup()
{
    Serial.begin(115200);
    delay(1000);

    i2s.setPins(
        26,  // BCLK
        25,  // LRCK / WS
        22   // DATA OUT
    );

    if (!i2s.begin(
        I2S_MODE_STD,
        44100,
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_STEREO,
        I2S_STD_SLOT_BOTH))
    {
        Serial.println("I2S initialization failed");
        while (true)
        {
            delay(1000);
        }
    }

    a2dpSink.start("Marocco's Lab Audio Receiver");

    Serial.println("Bluetooth A2DP receiver started");
}

void loop()
{
    delay(1000);
}