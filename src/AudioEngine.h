#pragma once
#include <Arduino.h>
#include <ESP_I2S.h>

extern I2SClass i2s;
void beginAudioEngine();
void audioCallback(const uint8_t* data, uint32_t length);

// Latest stereo PCM peaks (0..32767), updated by the audio callback.
extern volatile uint16_t stereoPeakLeft;
extern volatile uint16_t stereoPeakRight;

// Downsampled post-DSP stereo PCM for real oscilloscope traces.
// 512 frames x 2 channels, written in the audio callback; never alter audio.
constexpr uint16_t SCOPE_RING_SIZE = 512;
extern volatile int16_t scopeRingL[SCOPE_RING_SIZE];
extern volatile int16_t scopeRingR[SCOPE_RING_SIZE];
extern volatile uint16_t scopeWriteIndex;
