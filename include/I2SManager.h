#pragma once

#include <Arduino.h>
#include <ESP_I2S.h>

class I2SManager
{
public:
    bool begin();
    size_t write(const uint8_t* data, size_t length);

private:
    I2SClass m_i2s;
};