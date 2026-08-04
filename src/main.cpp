#include <Arduino.h>
#include "BluetoothManager.h"

BluetoothManager bluetooth;

void setup()
{
    Serial.begin(115200);

    bluetooth.begin();
}

void loop()
{
}