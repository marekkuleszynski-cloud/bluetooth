#include <Arduino.h>

#include "AudioBuffer.h"
#include "BluetoothManager.h"
#include "I2SManager.h"
#include "AudioManager.h"

AudioBuffer audioBuffer;
BluetoothManager bluetooth(audioBuffer);

I2SManager i2s;
AudioManager audio(audioBuffer, i2s);

void setup()
{
    Serial.begin(115200);
    delay(5000);

    Serial.println("=== SYSTEM START ===");

    if (!audioBuffer.begin(8192))
    {
        Serial.println("ERROR: AudioBuffer initialization failed");
        return;
    }

    Serial.println("AudioBuffer initialized");

    if (!i2s.begin())
    {
        Serial.println("ERROR: I2S initialization failed");
        return;
    }

    Serial.println("I2S initialized");

    audio.begin();

    bluetooth.begin();

    Serial.println("Bluetooth started");
}

void loop()
{
    audio.process();
}