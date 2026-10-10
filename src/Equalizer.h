#pragma once

#include <Arduino.h>

// Shared sampling frequency: used by the equalizer and the FFT/audio path.
// Keep this at the same value as the existing I2S configuration.
constexpr float SAMPLE_RATE = 44100.0f;

constexpr uint8_t EQ_BAND_COUNT = 5;

struct BiquadCoeffs
{
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
};

struct BiquadState
{
    float z1 = 0.0f;
    float z2 = 0.0f;
};

extern const char* eqBandNames[EQ_BAND_COUNT];
extern float eqGain[EQ_BAND_COUNT];

extern BiquadCoeffs eqCoeffBank[2][EQ_BAND_COUNT];
extern bool eqActiveBank[2][EQ_BAND_COUNT];
extern float eqPreampBank[2];
extern volatile uint8_t activeEqBank;
extern BiquadState eqStateL[EQ_BAND_COUNT];
extern BiquadState eqStateR[EQ_BAND_COUNT];

// Kept inline exactly as before: this runs for every active biquad/sample.
inline float processBiquad(
    float input,
    const BiquadCoeffs& c,
    BiquadState& s
)
{
    float output =
        c.b0 * input + s.z1;

    s.z1 =
        c.b1 * input -
        c.a1 * output +
        s.z2;

    s.z2 =
        c.b2 * input -
        c.a2 * output;

    return output;
}

void updateEQCoefficients();
