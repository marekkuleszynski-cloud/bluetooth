#include <Arduino.h>

void setup()
{
    Serial.begin(115200);

    Serial.println("ESP32 initiated");
}

void loop()
{
    Serial.println("Working correctly");

    delay(5000);
}