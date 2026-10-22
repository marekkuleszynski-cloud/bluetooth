#include "AudioEngine.h"
#include "HardwareConfig.h"
#include "AppState.h"
#include "Equalizer.h"
#include "SpectrumAnalyzer.h"
#include <cstring>

I2SClass i2s;
volatile uint16_t stereoPeakLeft = 0;
volatile uint16_t stereoPeakRight = 0;
volatile int16_t scopeRingL[SCOPE_RING_SIZE] = {};
volatile int16_t scopeRingR[SCOPE_RING_SIZE] = {};
volatile uint16_t scopeWriteIndex = 0;
// Visual-only AGC: one stereo-linked reference, never applied to I2S.
// V1 stopped adapting at -58 dBFS and saturated at x24. Quiet Bluetooth
// volume settings therefore remained visibly quieter on every display.
// 16-bit PCM cannot be recovered when the phone transmits digital zero.
constexpr float VISUAL_TARGET_RMS = 0.13f;
constexpr float VISUAL_MIN_INPUT_RMS = 0.00003f; // ~ -90.5 dBFS
constexpr float VISUAL_GAIN_MIN = 0.05f;
constexpr float VISUAL_GAIN_MAX = 512.0f;
constexpr float VISUAL_LEVEL_RISE_MS = 450.0f;
constexpr float VISUAL_LEVEL_FALL_MS = 850.0f;
constexpr float VISUAL_SILENCE_RESET_MS = 2500.0f;

static float visualRmsReference = 0.0f;
static float visualGain = 1.0f;
static float visualQuietMs = 0.0f;
static inline int16_t visualSample(int16_t sample, float gain)
{
    float v = static_cast<float>(sample) * gain;
    if (v > 32767.0f) v = 32767.0f;
    if (v < -32768.0f) v = -32768.0f;
    return static_cast<int16_t>(v);
}

// ==================================================
// AUDIO PROCESSING / TRUE PCM BYPASS
// ==================================================

// true  = preamp + 5-band EQ
// false = raw A2DP PCM is copied directly to I2S
// Shared flag is defined in AppState.cpp.

// Crossfade time between processed and raw PCM.
// No mute / no I2S restart is used.
// SMART BOOST stereo-linked, zero-lookahead peak limiter.
// One identical gain for L and R maintains stereo balance.
// No constant preamp reduction: gain is unity unless the PCM peak
// would exceed the ceiling. Instant peak protection, slow release.
constexpr float SMART_LIMIT_CEILING = 0.98f;  // ~ -0.18 dBFS
constexpr float SMART_LIMIT_RELEASE_MS = 160.0f;
constexpr float SMART_LIMIT_RELEASE_STEP =
    1.0f / (SAMPLE_RATE * SMART_LIMIT_RELEASE_MS / 1000.0f);

constexpr float PROCESSING_CROSSFADE_MS = 20.0f;
constexpr float PROCESSING_CROSSFADE_FRAMES =
    SAMPLE_RATE *
    PROCESSING_CROSSFADE_MS /
    1000.0f;

constexpr float PROCESSING_CROSSFADE_STEP =
    1.0f /
    PROCESSING_CROSSFADE_FRAMES;


inline int16_t floatToSample(

    float value

)

{

    value =

        constrain(

            value,

            -1.0f,

            0.999969f

        );

    return static_cast<int16_t>(

        value * 32767.0f

    );

}
void audioCallback(

    const uint8_t* data,

    uint32_t length

)

{

    const int16_t* input =

        reinterpret_cast<

            const int16_t*

        >(data);

    size_t sampleCount =

        length /

        sizeof(int16_t);

    sampleCount &= ~1UL;

    constexpr size_t CHUNK_FRAMES =

        128;

    constexpr size_t CHUNK_SAMPLES =

        CHUNK_FRAMES * 2;

    static int16_t output[

        CHUNK_SAMPLES

    ];

    // 1.0 = fully processed
    // 0.0 = true raw PCM bypass
    static float processingMix = 1.0f;
    static bool previousProcessingTarget = true;
    static float limiterGain = 1.0f; // Persistent across audio chunks


    size_t offset = 0;

    float callbackPeak = 0.0f;
    uint16_t blockPeakL = 0, blockPeakR = 0;

    while (offset < sampleCount)

    {

        size_t count =

            sampleCount -

            offset;

        if (count >

            CHUNK_SAMPLES)

        {

            count =

                CHUNK_SAMPLES;

        }

        count &= ~1UL;

        bool processingTarget =

            audioProcessingEnabled;

        // When processing is enabled again after a real bypass,
        // restart the filter states cleanly. The following crossfade
        // prevents a click when the processed path is introduced.
        if (
            processingTarget &&
            !previousProcessingTarget
        )
        {
            memset(
                eqStateL,
                0,
                sizeof(eqStateL)
            );

            memset(
                eqStateR,
                0,
                sizeof(eqStateR)
            );
        }

        previousProcessingTarget =

            processingTarget;

        uint8_t bank =

            activeEqBank;

        float preamp =

            eqPreampBank[bank];

        double visualEnergy = 0.0;

        for (size_t i = 0;

             i < count;

             i += 2)

        {

            const int16_t rawL =

                input[offset + i];

            const int16_t rawR =

                input[offset + i + 1];

            float rawLeft =

                rawL /

                32768.0f;

            float rawRight =

                rawR /

                32768.0f;

            float samplePeak =

                max(

                    fabsf(rawLeft),

                    fabsf(rawRight)

                );

            if (samplePeak >

                callbackPeak)

            {

                callbackPeak =

                    samplePeak;

            }

            // Move continuously toward the requested mode.
            if (processingTarget)

            {

                processingMix =

                    min(

                        1.0f,

                        processingMix +

                        PROCESSING_CROSSFADE_STEP

                    );

            }

            else

            {

                processingMix =

                    max(

                        0.0f,

                        processingMix -

                        PROCESSING_CROSSFADE_STEP

                    );

            }

            int16_t outL;
            int16_t outR;

            if (
                !processingTarget &&
                processingMix <= 0.0f
            )
            {
                // TRUE BYPASS:
                // exact A2DP PCM samples go directly to I2S.
                // No preamp, float conversion, EQ or clamp.
                outL = rawL;
                outR = rawR;
                limiterGain = 1.0f;
            }
            else
            {
                float processedLeft =

                    rawLeft *

                    preamp;

                float processedRight =

                    rawRight *

                    preamp;

                for (uint8_t stage = 0;

                     stage < EQ_BAND_COUNT;

                     stage++)

                {

                    if (!eqActiveBank[

                            bank

                        ][stage])

                    {

                        continue;

                    }

                    processedLeft =

                        processBiquad(

                            processedLeft,

                            eqCoeffBank[

                                bank

                            ][stage],

                            eqStateL[stage]

                        );

                    processedRight =

                        processBiquad(

                            processedRight,

                            eqCoeffBank[

                                bank

                            ][stage],

                            eqStateR[stage]

                        );

                }

                // Crossfade raw <-> processed before limiting.
                float mixedLeft = processedLeft;
                float mixedRight = processedRight;
                if (processingMix < 1.0f)
                {
                    mixedLeft = rawLeft * (1.0f - processingMix) +
                                processedLeft * processingMix;
                    mixedRight = rawRight * (1.0f - processingMix) +
                                 processedRight * processingMix;
                }

                // Protect against unstable filter output (e.g. NaN).
                if (!isfinite(mixedLeft)) mixedLeft = 0.0f;
                if (!isfinite(mixedRight)) mixedRight = 0.0f;

                // Stereo-linked peak limiter. For a transient that would
                // clip, reduce both channels by exactly the same factor.
                const float peak = fmaxf(fabsf(mixedLeft), fabsf(mixedRight));
                const float requiredGain = peak > SMART_LIMIT_CEILING
                    ? SMART_LIMIT_CEILING / peak : 1.0f;

                if (requiredGain < limiterGain)
                {
                    limiterGain = requiredGain; // fast attack (no overshoot)
                }
                else
                {
                    // Gentle gain recovery over ~160 ms. In the meantime,
                    // keep enough protection for the current peak.
                    limiterGain = fminf(requiredGain,
                                        limiterGain + SMART_LIMIT_RELEASE_STEP);
                }

                outL = floatToSample(mixedLeft * limiterGain);
                outR = floatToSample(mixedRight * limiterGain);
            }

            output[i] =

                outL;

            output[i + 1] =

                outR;

            // Stereo-linked RMS for visual AGC, before any display scaling.
            const float normL = outL * (1.0f / 32768.0f);
            const float normR = outR * (1.0f / 32768.0f);
            visualEnergy += 0.5 * (double(normL) * normL + double(normR) * normR);


            // Non-blocking post-DSP oscilloscope tap, ~5.5 kframes/s.
            // Single producer; each 16-bit PCM store is atomic on ESP32.
            static uint8_t scopeDecimator = 0;
            if (++scopeDecimator >= 8)
            {
                scopeDecimator = 0;
                const uint16_t wi = scopeWriteIndex;
                // Scope keeps RAW post-DSP PCM: normalize each display window
                // separately to avoid 16-bit clipping in Visual AGC.
                scopeRingL[wi] = outL;
                scopeRingR[wi] = outR;
                scopeWriteIndex = (wi + 1) & (SCOPE_RING_SIZE - 1);
            }

            // Meter taps after DSP/bypass, exactly what reaches the DAC.
            const int16_t meterL = visualSample(outL, visualGain);
            const int16_t meterR = visualSample(outR, visualGain);
            const uint16_t aL = meterL == INT16_MIN ? 32768u : abs(meterL);
            const uint16_t aR = meterR == INT16_MIN ? 32768u : abs(meterR);
            if (aL > blockPeakL) blockPeakL = aL;
            if (aR > blockPeakR) blockPeakR = aR;


            // FFT always follows what is actually sent to the DAC.
            if (!fftReady)

            {

                vReal[fftIndex] =
                    (static_cast<double>(meterL) +
                     static_cast<double>(meterR)) * 0.5;

                vImag[fftIndex] =

                    0.0;

                fftIndex = fftIndex + 1;

                if (fftIndex >=

                    FFT_SIZE)

                {

                    fftIndex = 0;

                    fftReady = true;

                }

            }

        }

        // Shared stereo RMS reference. Update only once per PCM chunk.
        // Quiet chunks must not immediately erase the level reference:
        // real music contains pauses and near-zero crossings.
        const float frames = count * 0.5f;
        const float chunkMs = frames * (1000.0f / SAMPLE_RATE);
        const float chunkRms = sqrtf(static_cast<float>(visualEnergy / frames));
        if (chunkRms >= VISUAL_MIN_INPUT_RMS)
        {
            visualQuietMs = 0.0f;
            if (visualRmsReference < VISUAL_MIN_INPUT_RMS)
                visualRmsReference = chunkRms;
            else
            {
                const float tauMs = chunkRms > visualRmsReference
                    ? VISUAL_LEVEL_RISE_MS : VISUAL_LEVEL_FALL_MS;
                const float follow = 1.0f - expf(-chunkMs / tauMs);
                visualRmsReference += (chunkRms - visualRmsReference) * follow;
            }
            visualGain = constrain(VISUAL_TARGET_RMS / visualRmsReference,
                                   VISUAL_GAIN_MIN, VISUAL_GAIN_MAX);
        }
        else
        {
            visualQuietMs += chunkMs;
            if (visualQuietMs >= VISUAL_SILENCE_RESET_MS)
            {
                // In actual silence visualSample(0, gain) remains zero;
                // reset after a pause so the next track recalibrates.
                visualRmsReference = 0.0f;
                visualGain = 1.0f;
            }
        }

        // Visual meter only: one store per callback/chunk (no TFT in audio).
        stereoPeakLeft = blockPeakL;
        stereoPeakRight = blockPeakR;

        i2s.write(

            output,

            count *

            sizeof(int16_t)

        );

        offset += count;

    }

    if (callbackPeak > AUDIO_SILENCE_THRESHOLD)
    {
        lastAudioSignalMs = millis();
    }

}
void beginAudioEngine()
{
    i2s.setPins(I2S_BCLK,I2S_LRCK,I2S_DATA);
    if (!i2s.begin(I2S_MODE_STD,44100,I2S_DATA_BIT_WIDTH_16BIT,
                   I2S_SLOT_MODE_STEREO,I2S_STD_SLOT_BOTH))
    {
        Serial.println("I2S initialization failed");
        while (true) delay(1000);
    }
}
