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
// ==================================================
// AUDIO PROCESSING / TRUE PCM BYPASS
// ==================================================

// true  = preamp + 5-band EQ
// false = raw A2DP PCM is copied directly to I2S
// Shared flag is defined in AppState.cpp.

// Crossfade time between processed and raw PCM.
// No mute / no I2S restart is used.
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

                if (processingMix >= 1.0f)
                {
                    outL =
                        floatToSample(
                            processedLeft
                        );

                    outR =
                        floatToSample(
                            processedRight
                        );
                }
                else
                {
                    // Smooth, sample-by-sample crossfade.
                    float mixedLeft =

                        rawLeft *

                            (1.0f - processingMix) +

                        processedLeft *

                            processingMix;

                    float mixedRight =

                        rawRight *

                            (1.0f - processingMix) +

                        processedRight *

                            processingMix;

                    outL =
                        floatToSample(
                            mixedLeft
                        );

                    outR =
                        floatToSample(
                            mixedRight
                        );
                }
            }

            output[i] =

                outL;

            output[i + 1] =

                outR;

            // Non-blocking post-DSP oscilloscope tap, ~5.5 kframes/s.
            // Single producer; each 16-bit PCM store is atomic on ESP32.
            static uint8_t scopeDecimator = 0;
            if (++scopeDecimator >= 8)
            {
                scopeDecimator = 0;
                const uint16_t wi = scopeWriteIndex;
                scopeRingL[wi] = outL;
                scopeRingR[wi] = outR;
                scopeWriteIndex = (wi + 1) & (SCOPE_RING_SIZE - 1);
            }

            // Meter taps after DSP/bypass, exactly what reaches the DAC.
            const uint16_t aL = outL == INT16_MIN ? 32768u : abs(outL);
            const uint16_t aR = outR == INT16_MIN ? 32768u : abs(outR);
            if (aL > blockPeakL) blockPeakL = aL;
            if (aR > blockPeakR) blockPeakR = aR;


            // FFT always follows what is actually sent to the DAC.
            if (!fftReady)

            {

                vReal[fftIndex] =

                    (

                        static_cast<double>(

                            outL

                        ) +

                        static_cast<double>(

                            outR

                        )

                    ) * 0.5;

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
