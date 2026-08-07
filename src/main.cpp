#include <Arduino.h>

#include "BluetoothManager.h"
#include "AudioManager.h"

BluetoothManager bluetooth;
AudioManager audio;

void setup()
{
    Serial.begin(115200);

    delay(5000);

    Serial.println("System start");

    bluetooth.begin();
    audio.begin();

    Serial.println("Bluetooth started");
}

void loop()
{
}