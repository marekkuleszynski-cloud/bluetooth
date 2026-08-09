#include <Arduino.h>

#include "AudioBuffer.h"
#include "BluetoothManager.h"
#include "AudioManager.h"

AudioBuffer audioBuffer;
BluetoothManager bluetooth(audioBuffer);
AudioManager audio;

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

    audio.begin();

    bluetooth.begin();

    Serial.println("Bluetooth started");
}

void loop()
{
}