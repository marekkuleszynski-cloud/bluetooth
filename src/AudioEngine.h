#pragma once
#include <Arduino.h>
#include <ESP_I2S.h>

extern I2SClass i2s;
void beginAudioEngine();
void audioCallback(const uint8_t* data, uint32_t length);

// Latest stereo PCM peaks (0..32767), updated by the audio callback.
extern volatile uint16_t stereoPeakLeft;
extern volatile uint16_t stereoPeakRight;
