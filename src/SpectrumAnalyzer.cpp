#include "SpectrumAnalyzer.h"
#include "Equalizer.h"
#include <arduinoFFT.h>

#include "AppState.h"

// ==================================================

// FFT

// ==================================================



double vReal[FFT_SIZE];

double vImag[FFT_SIZE];

ArduinoFFT<double> FFT(

    vReal,

    vImag,

    FFT_SIZE,

    SAMPLE_RATE

);

volatile uint16_t fftIndex = 0;

volatile bool fftReady = false;

// ==================================================

// SPECTRUM

// ==================================================





float bands[BAND_COUNT] = {0};

float smoothedBands[BAND_COUNT] = {0};

float autoPeak = 1.0f;

// Visualization-only frequency balance. These factors do NOT change PCM or EQ.
// Bass is attenuated gently relative to mids/highs; adjust gradually if needed.
constexpr float DISPLAY_BAND_GAIN[BAND_COUNT] =
{
    0.78f, 0.82f, 0.87f, 0.92f,
    0.96f, 1.00f, 1.00f, 1.00f,
    1.00f, 1.00f, 1.00f, 1.00f,
    1.00f, 1.00f, 1.00f, 1.00f
};

// Extra visual headroom makes it harder for the tallest bar to remain
// pinned to 100%. This affects only the shared visual scale.
constexpr float DISPLAY_HEADROOM = 1.12f;

// RTA-oriented 16-band division for FFT_SIZE=1024 / 44.1 kHz.
// One FFT bin is about 43.07 Hz.
const float rtaBandEdges[BAND_COUNT + 1] =
{
    43.07f,
    86.13f,
    129.20f,
    172.27f,
    258.40f,
    344.53f,
    473.73f,
    689.06f,
    990.53f,
    1421.19f,
    2024.12f,
    2842.38f,
    4005.18f,
    5684.77f,
    8053.42f,
    11369.53f,
    22050.0f
};

// ==================================================

// FFT

// ==================================================

void calculateRTABands()
{
    constexpr float BIN_WIDTH =
        SAMPLE_RATE / FFT_SIZE;

    for (uint8_t band = 0;
         band < BAND_COUNT;
         band++)
    {
        // Half-open interval [startBin, endBinExclusive)
        // prevents neighbouring bands from sharing the same FFT bin.
        uint16_t startBin =
            static_cast<uint16_t>(
                ceilf(
                    rtaBandEdges[band] /
                    BIN_WIDTH
                )
            );

        uint16_t endBinExclusive =
            static_cast<uint16_t>(
                ceilf(
                    rtaBandEdges[band + 1] /
                    BIN_WIDTH
                )
            );

        if (startBin < 1)
        {
            startBin = 1;
        }

        if (endBinExclusive >
            FFT_SIZE / 2)
        {
            endBinExclusive =
                FFT_SIZE / 2;
        }

        if (endBinExclusive <=
            startBin)
        {
            endBinExclusive =
                startBin + 1;
        }

        if (endBinExclusive >
            FFT_SIZE / 2)
        {
            endBinExclusive =
                FFT_SIZE / 2;
        }

        double sumSquares = 0.0;
        uint16_t binCount = 0;

        for (uint16_t bin = startBin;
             bin < endBinExclusive;
             bin++)
        {
            double magnitude =
                vReal[bin];

            sumSquares +=
                magnitude *
                magnitude;

            binCount++;
        }

        if (binCount > 0)
        {
            // RTA: integrated power across the frequency band.
            // Unlike average power per FFT bin, this preserves
            // the energy contributed by the band's bandwidth.
            // Keep the existing log display mapping in this step.
            double rmsMagnitude = sqrt(sumSquares);

            bands[band] =
                log10f(
                    static_cast<float>(
                        rmsMagnitude
                    ) +
                    1.0f
                );
        }
        else
        {
            bands[band] = 0.0f;
        }
    }
}

void processFFT()

{

    FFT.windowing(

        FFT_WIN_TYP_HAMMING,

        FFT_FORWARD

    );

    FFT.compute(

        FFT_FORWARD

    );

    FFT.complexToMagnitude();

    // All visual modes use the same RTA frequency partition.
    // They differ only in rendering and smoothing.
    calculateRTABands();

    // Apply the visual correction after FFT power integration; no audio change.
    for (uint8_t i = 0; i < BAND_COUNT; ++i)
    {
        bands[i] *= DISPLAY_BAND_GAIN[i];
    }

    float framePeak = 0.0f;

    for (uint8_t i = 0;

         i < BAND_COUNT;

         i++)

    {

        framePeak =

            max(

                framePeak,

                bands[i]

            );

    }

    if (framePeak * DISPLAY_HEADROOM >

        autoPeak)

    {

        autoPeak =

            framePeak * DISPLAY_HEADROOM;

    }

    else

    {

        autoPeak *=

            0.990f;

    }

    if (autoPeak <

        1.0f)

    {

        autoPeak =

            1.0f;

    }

    float oldWeight =

        0.45f;

    float newWeight =

        0.55f;

    if (

        visualMode ==

        NEON_DYNAMIC

    )

    {

        oldWeight =

            0.18f;

        newWeight =

            0.82f;

    }

    else if (

        visualMode ==

            MIRROR_LED ||

        visualMode ==

            RAINBOW_LED ||

        visualMode ==

            VERTICAL_RAINBOW ||

        visualMode ==

            COLOR_WAVE ||

        visualMode ==

            PIXEL_MATRIX

    )

    {

        oldWeight =

            0.28f;

        newWeight =

            0.72f;

    }

    for (uint8_t i = 0;

         i < BAND_COUNT;

         i++)

    {

        smoothedBands[i] =

            smoothedBands[i] *

                oldWeight +

            bands[i] *

                newWeight;

    }

}

// ==================================================

// 12-BAND MAPPING

// ==================================================

float get12Band(

    uint8_t band,

    float exponent

)

{

    uint8_t first =

        band *

        BAND_COUNT /

        BAND12_COUNT;

    uint8_t end =

        (band + 1) *

        BAND_COUNT /

        BAND12_COUNT;

    if (end <= first)

    {

        end =

            first + 1;

    }

    float value =

        0.0f;

    for (uint8_t i = first;

         i < end &&

         i < BAND_COUNT;

         i++)

    {

        value =

            max(

                value,

                smoothedBands[i]

            );

    }

    value /=

        autoPeak;

    value =

        constrain(

            value,

            0.0f,

            1.0f

        );

    return powf(

        value,

        exponent

    );

}

