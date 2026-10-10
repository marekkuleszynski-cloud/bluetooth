#pragma once

#include <Arduino.h>
#include "VisualMode.h"

constexpr uint16_t FFT_SIZE = 1024;
constexpr uint8_t BAND_COUNT = 16;
constexpr uint8_t BAND12_COUNT = 12;

// Double-buffering/real-time synchronization is unchanged in this refactor.
// audioCallback writes vReal/vImag; loop processes when fftReady is set.
extern double vReal[FFT_SIZE];
extern double vImag[FFT_SIZE];
extern volatile uint16_t fftIndex;
extern volatile bool fftReady;

extern float bands[BAND_COUNT];
extern float smoothedBands[BAND_COUNT];
extern float autoPeak;

void calculateRTABands();
void processFFT();
float get12Band(uint8_t band, float exponent);
