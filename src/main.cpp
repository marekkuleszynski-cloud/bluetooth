#include <Arduino.h>

#include <ESP_I2S.h>

#include <BluetoothA2DPSink.h>

#include <TFT_eSPI.h>

#include <arduinoFFT.h>

// ==================================================

// HARDWARE

// ==================================================

I2SClass i2s;

BluetoothA2DPSink a2dpSink(i2s);

TFT_eSPI tft;

// I2S

constexpr int I2S_BCLK = 26;

constexpr int I2S_LRCK = 25;

constexpr int I2S_DATA = 22;

// Encoder

constexpr int ENC_A   = 32;

constexpr int ENC_B   = 33;

constexpr int ENC_BTN = 21;

// KEY0: audio processing ON/OFF
constexpr int KEY0_PIN = 19;

// ==================================================

// AUDIO

// ==================================================

constexpr float SAMPLE_RATE = 44100.0f;

// ==================================================
// AUDIO PROCESSING / TRUE PCM BYPASS
// ==================================================

// true  = preamp + 5-band EQ
// false = raw A2DP PCM is copied directly to I2S
volatile bool audioProcessingEnabled = true;

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

// KEY0 debounce
bool lastKey0State = HIGH;
bool stableKey0State = HIGH;
unsigned long lastKey0Change = 0;
constexpr unsigned long KEY0_DEBOUNCE_MS = 35;

// ==================================================

// STARTUP STATE

// ==================================================

bool waitingForAudio = true;

// Bluetooth / audio-signal UI state
volatile bool btConnected = false;
volatile bool btConnectionChanged = false;
volatile uint32_t lastAudioSignalMs = 0;

// Real audio signal detector.
// 0.002 ~= -54 dBFS.
constexpr float AUDIO_SILENCE_THRESHOLD = 0.0020f;
constexpr unsigned long AUDIO_SIGNAL_TIMEOUT_MS = 500;

enum SignalScreenState
{
    SCREEN_WAITING_BT,
    SCREEN_WAITING_AUDIO,
    SCREEN_RUNNING
};

SignalScreenState signalScreenState = SCREEN_WAITING_BT;

constexpr const char* TEXT_WAITING_BT =
    "Waiting for BT connection";

constexpr const char* TEXT_WAITING_AUDIO =
    "Waiting for audio signal";

// ==================================================

// REAL 5-BAND DSP EQUALIZER

// ==================================================

constexpr uint8_t EQ_BAND_COUNT = 5;

const char* eqBandNames[EQ_BAND_COUNT] =

{

    "80",

    "250",

    "1k",

    "4k",

    "10k"

};

float eqGain[EQ_BAND_COUNT] =

{

    0.0f,

    0.0f,

    0.0f,

    0.0f,

    0.0f

};

// ==================================================

// BIQUAD

// ==================================================

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

BiquadCoeffs eqCoeffBank[2][EQ_BAND_COUNT];

bool eqActiveBank[2][EQ_BAND_COUNT];

float eqPreampBank[2] =

{

    1.0f,

    1.0f

};

volatile uint8_t activeEqBank = 0;

BiquadState eqStateL[EQ_BAND_COUNT];

BiquadState eqStateR[EQ_BAND_COUNT];

// ==================================================

// BIQUAD PROCESSING

// ==================================================

inline float processBiquad(

    float input,

    const BiquadCoeffs& c,

    BiquadState& s

)

{

    float output =

        c.b0 * input +

        s.z1;

    s.z1 =

        c.b1 * input -

        c.a1 * output +

        s.z2;

    s.z2 =

        c.b2 * input -

        c.a2 * output;

    return output;

}

void normalizeCoeffs(

    BiquadCoeffs& c,

    float a0

)

{

    c.b0 /= a0;

    c.b1 /= a0;

    c.b2 /= a0;

    c.a1 /= a0;

    c.a2 /= a0;

}

// ==================================================

// PEAKING FILTER

// ==================================================

BiquadCoeffs makePeaking(

    float frequency,

    float q,

    float gainDb

)

{

    float A =

        powf(

            10.0f,

            gainDb / 40.0f

        );

    float omega =

        2.0f *

        PI *

        frequency /

        SAMPLE_RATE;

    float sn =

        sinf(omega);

    float cs =

        cosf(omega);

    float alpha =

        sn /

        (2.0f * q);

    BiquadCoeffs c;

    c.b0 =

        1.0f +

        alpha * A;

    c.b1 =

        -2.0f * cs;

    c.b2 =

        1.0f -

        alpha * A;

    float a0 =

        1.0f +

        alpha / A;

    c.a1 =

        -2.0f * cs;

    c.a2 =

        1.0f -

        alpha / A;

    normalizeCoeffs(

        c,

        a0

    );

    return c;

}

// ==================================================

// LOW SHELF

// ==================================================

BiquadCoeffs makeLowShelf(

    float frequency,

    float gainDb

)

{

    float A =

        powf(

            10.0f,

            gainDb / 40.0f

        );

    float omega =

        2.0f *

        PI *

        frequency /

        SAMPLE_RATE;

    float sn =

        sinf(omega);

    float cs =

        cosf(omega);

    constexpr float S = 1.0f;

    float alpha =

        sn * 0.5f *

        sqrtf(

            (A + 1.0f / A) *

            (1.0f / S - 1.0f) +

            2.0f

        );

    float beta =

        2.0f *

        sqrtf(A) *

        alpha;

    BiquadCoeffs c;

    c.b0 =

        A *

        (

            (A + 1.0f) -

            (A - 1.0f) * cs +

            beta

        );

    c.b1 =

        2.0f *

        A *

        (

            (A - 1.0f) -

            (A + 1.0f) * cs

        );

    c.b2 =

        A *

        (

            (A + 1.0f) -

            (A - 1.0f) * cs -

            beta

        );

    float a0 =

        (A + 1.0f) +

        (A - 1.0f) * cs +

        beta;

    c.a1 =

        -2.0f *

        (

            (A - 1.0f) +

            (A + 1.0f) * cs

        );

    c.a2 =

        (A + 1.0f) +

        (A - 1.0f) * cs -

        beta;

    normalizeCoeffs(

        c,

        a0

    );

    return c;

}

// ==================================================

// HIGH SHELF

// ==================================================

BiquadCoeffs makeHighShelf(

    float frequency,

    float gainDb

)

{

    float A =

        powf(

            10.0f,

            gainDb / 40.0f

        );

    float omega =

        2.0f *

        PI *

        frequency /

        SAMPLE_RATE;

    float sn =

        sinf(omega);

    float cs =

        cosf(omega);

    constexpr float S = 1.0f;

    float alpha =

        sn * 0.5f *

        sqrtf(

            (A + 1.0f / A) *

            (1.0f / S - 1.0f) +

            2.0f

        );

    float beta =

        2.0f *

        sqrtf(A) *

        alpha;

    BiquadCoeffs c;

    c.b0 =

        A *

        (

            (A + 1.0f) +

            (A - 1.0f) * cs +

            beta

        );

    c.b1 =

        -2.0f *

        A *

        (

            (A - 1.0f) +

            (A + 1.0f) * cs

        );

    c.b2 =

        A *

        (

            (A + 1.0f) +

            (A - 1.0f) * cs -

            beta

        );

    float a0 =

        (A + 1.0f) -

        (A - 1.0f) * cs +

        beta;

    c.a1 =

        2.0f *

        (

            (A - 1.0f) -

            (A + 1.0f) * cs

        );

    c.a2 =

        (A + 1.0f) -

        (A - 1.0f) * cs -

        beta;

    normalizeCoeffs(

        c,

        a0

    );

    return c;

}

// ==================================================

// UPDATE EQ

// ==================================================

void updateEQCoefficients()

{

    uint8_t newBank =

        1 - activeEqBank;

    eqCoeffBank[newBank][0] =

        makeLowShelf(

            80.0f,

            eqGain[0]

        );

    eqCoeffBank[newBank][1] =

        makePeaking(

            250.0f,

            1.0f,

            eqGain[1]

        );

    eqCoeffBank[newBank][2] =

        makePeaking(

            1000.0f,

            1.0f,

            eqGain[2]

        );

    eqCoeffBank[newBank][3] =

        makePeaking(

            4000.0f,

            1.0f,

            eqGain[3]

        );

    eqCoeffBank[newBank][4] =

        makeHighShelf(

            10000.0f,

            eqGain[4]

        );

    float maxBoost = 0.0f;

    for (uint8_t i = 0;

         i < EQ_BAND_COUNT;

         i++)

    {

        eqActiveBank[newBank][i] =

            fabsf(eqGain[i]) >

            0.05f;

        if (eqGain[i] >

            maxBoost)

        {

            maxBoost =

                eqGain[i];

        }

    }

    if (maxBoost > 0.0f)

    {

        eqPreampBank[newBank] =

            powf(

                10.0f,

                -(maxBoost + 1.0f) /

                20.0f

            );

    }

    else

    {

        eqPreampBank[newBank] =

            1.0f;

    }

    activeEqBank =

        newBank;

}

// ==================================================

// FFT

// ==================================================

constexpr uint16_t FFT_SIZE = 1024;

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

constexpr uint8_t BAND_COUNT = 16;

constexpr uint8_t BAND12_COUNT = 12;

float bands[BAND_COUNT] = {0};

float smoothedBands[BAND_COUNT] = {0};

float autoPeak = 1.0f;

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

// MODES

// ==================================================

enum VisualMode

{

    CLASSIC_LED = 0,

    NEON_DYNAMIC,

    CENTER_BARS,

    MIRROR_LED,

    PEAK_HOLD,

    RAINBOW_LED,

    RETRO_GREEN,

    VERTICAL_RAINBOW,

    COLOR_WAVE,

    PIXEL_MATRIX,

    EQUALIZER,

    MODE_COUNT

};

const char* visualModeNames[MODE_COUNT] =
{
    "Classic LED",
    "Neon Dynamic",
    "Center Bars",
    "Mirror LED",
    "Peak Hold",
    "Rainbow LED",
    "Retro Green",
    "Vertical Rainbow",
    "Color Wave",
    "Pixel Matrix",
    "Equalizer"
};

VisualMode visualMode =

    CLASSIC_LED;

enum UiState

{

    PLAYBACK_MODE,

    EQ_EDIT_MODE

};

UiState uiState =

    PLAYBACK_MODE;

uint8_t selectedEqBand = 0;

// ==================================================

// DRAW STATE

// ==================================================

int previousClassic[BAND_COUNT] = {0};

int previousNeon[BAND_COUNT] = {0};

int previousCenter[BAND_COUNT] = {0};

int previousMirror[BAND_COUNT] = {0};

int previousPeak[BAND_COUNT] = {0};

int previousRainbow[BAND_COUNT] = {0};

int previousPeakMarker[BAND_COUNT] = {0};

float peakHoldPosition[BAND_COUNT] =

{

    0

};

int previousRetro[BAND12_COUNT] = {0};

int previousVertical[BAND12_COUNT] = {0};

int previousWave[BAND12_COUNT] = {0};

int previousMatrix[BAND12_COUNT] = {0};

// ==================================================

// ENCODER

// ==================================================

volatile int8_t encoderAccumulator = 0;

volatile int8_t encoderPendingSteps = 0;

volatile uint8_t encoderState = 3;

volatile bool encoderMovedFromDetent = false;

// ==================================================

// BUTTON

// ==================================================

bool lastButtonState = HIGH;

bool stableButtonState = HIGH;

unsigned long lastButtonChange = 0;

constexpr unsigned long BUTTON_DEBOUNCE_MS =

    35;

// ==================================================

// OVERLAY

// ==================================================

const char* overlayText = nullptr;
const char* overlayTextLine2 = nullptr;

unsigned long overlayStart = 0;

unsigned long lastOverlayDraw = 0;

constexpr unsigned long OVERLAY_DURATION =

    1800;

constexpr unsigned long OVERLAY_REFRESH =

    60;

// ==================================================

// EQ DISPLAY GEOMETRY

// ==================================================

constexpr int EQ_TOP = 31;

constexpr int EQ_BOTTOM = 193;

constexpr int EQ_CENTER_Y =

    (EQ_TOP + EQ_BOTTOM) / 2;

constexpr int EQ_RANGE =

    (EQ_BOTTOM - EQ_TOP) / 2;

constexpr int EQ_COLUMN_WIDTH = 42;

constexpr int eqCenters[EQ_BAND_COUNT] =

{

    40,

    100,

    160,

    220,

    280

};

// ==================================================

// SAMPLE CLAMP

// ==================================================

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

// ==================================================

// BLUETOOTH CONNECTION CALLBACK

// ==================================================

void btConnectionStateChanged(
    esp_a2d_connection_state_t state,
    void*
)
{
    if (state == ESP_A2D_CONNECTION_STATE_CONNECTED)
    {
        btConnected = true;
        btConnectionChanged = true;
    }
    else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
    {
        btConnected = false;
        btConnectionChanged = true;
    }
}

// ==================================================

// AUDIO CALLBACK

// A2DP -> EQ -> I2S -> FFT

// ==================================================

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

// ==================================================

// ENCODER ISR

// ==================================================

void IRAM_ATTR encoderISR()

{

    static const int8_t table[16] =

    {

         0, -1, +1,  0,

        +1,  0,  0, -1,

        -1,  0,  0, +1,

         0, +1, -1,  0

    };

    uint8_t a =

        digitalRead(ENC_A);

    uint8_t b =

        digitalRead(ENC_B);

    uint8_t newState =

        (a << 1) | b;

    if (newState ==

        encoderState)

    {

        return;

    }

    uint8_t transition =

        (encoderState << 2) |

        newState;

    encoderAccumulator +=

        table[transition];

    encoderState =

        newState;

    if (newState != 3)

    {

        encoderMovedFromDetent =

            true;

    }

    if (

        newState == 3 &&

        encoderMovedFromDetent

    )

    {

        if (encoderAccumulator >= 2)

        {

            encoderPendingSteps = encoderPendingSteps + 1;

        }

        else if (

            encoderAccumulator <= -2

        )

        {

            encoderPendingSteps = encoderPendingSteps - 1;

        }

        encoderAccumulator = 0;

        encoderMovedFromDetent =

            false;

    }

}

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
            double rmsMagnitude =
                sqrt(
                    sumSquares /
                    static_cast<double>(
                        binCount
                    )
                );

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

    if (framePeak >

        autoPeak)

    {

        autoPeak =

            framePeak;

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

// ==================================================

// COLORS

// ==================================================

uint16_t rainbowColor(

    uint8_t p

)

{

    if (p < 85)

    {

        uint8_t x =

            p * 3;

        return tft.color565(

            0,

            255 - x,

            x

        );

    }

    if (p < 170)

    {

        uint8_t x =

            (p - 85) * 3;

        return tft.color565(

            x,

            0,

            255 - x

        );

    }

    uint8_t x =

        (p - 170) * 3;

    return tft.color565(

        255,

        x,

        0

    );

}

uint16_t verticalColor(

    float level

)

{

    if (level < 0.35f)

        return TFT_CYAN;

    if (level < 0.65f)

        return TFT_GREEN;

    if (level < 0.82f)

        return TFT_YELLOW;

    if (level < 0.92f)

        return TFT_ORANGE;

    return TFT_RED;

}

// ==================================================

// HELPER: CLEAR DROPPED SEGMENTS

// ==================================================

void clearDroppedSegments(

    int* previous,

    int index,

    int active,

    int x,

    int width,

    int bottom,

    int step,

    int ledHeight,

    uint16_t background =

        TFT_BLACK

)

{

    if (active >=

        previous[index])

    {

        return;

    }

    for (int s = active;

         s < previous[index];

         s++)

    {

        int y =

            bottom -

            ((s + 1) *

             step);

        tft.fillRect(

            x,

            y,

            width,

            ledHeight,

            background

        );

    }

}

// ==================================================

// CLASSIC

// ==================================================

void drawClassicLed()

{

    constexpr int BOTTOM = 238;

    constexpr int STEP = 9;

    constexpr int LED_H = 7;

    constexpr int COUNT = 26;

    int barWidth =

        320 /

        BAND_COUNT;

    for (int i = 0;

         i < BAND_COUNT;

         i++)

    {

        float n =

            constrain(

                smoothedBands[i] /

                autoPeak,

                0.0f,

                1.0f

            );

        n =

            powf(

                n,

                1.8f

            );

        int active =

            constrain(

                static_cast<int>(

                    n * COUNT

                ),

                0,

                COUNT

            );

        int x =

            i * barWidth +

            1;

        int width =

            barWidth -

            3;

        clearDroppedSegments(

            previousClassic,

            i,

            active,

            x,

            width,

            BOTTOM,

            STEP,

            LED_H

        );

        for (int s = 0;

             s < active;

             s++)

        {

            float level =

                float(s + 1) /

                COUNT;

            uint16_t color;

            if (level < 0.70f)

                color = TFT_GREEN;

            else if (level < 0.90f)

                color = TFT_ORANGE;

            else

                color = TFT_RED;

            int y =

                BOTTOM -

                (s + 1) *

                STEP;

            tft.fillRect(

                x,

                y,

                width,

                LED_H,

                color

            );

        }

        previousClassic[i] =

            active;

    }

}

// ==================================================

// NEON

// ==================================================

void drawNeonDynamic()

{

    constexpr int BOTTOM = 238;

    constexpr int STEP = 7;

    constexpr int LED_H = 5;

    constexpr int COUNT = 33;

    int barWidth =

        320 /

        BAND_COUNT;

    for (int i = 0;

         i < BAND_COUNT;

         i++)

    {

        float n =

            constrain(

                smoothedBands[i] /

                autoPeak,

                0.0f,

                1.0f

            );

        n =

            powf(

                n,

                2.45f

            );

        int active =

            constrain(

                int(

                    n * COUNT

                ),

                0,

                COUNT

            );

        int x =

            i * barWidth +

            1;

        int width =

            barWidth -

            2;

        clearDroppedSegments(

            previousNeon,

            i,

            active,

            x,

            width,

            BOTTOM,

            STEP,

            LED_H

        );

        for (int s = 0;

             s < active;

             s++)

        {

            float level =

                float(s + 1) /

                COUNT;

            uint16_t color;

            if (level < 0.50f)

                color = TFT_CYAN;

            else if (level < 0.75f)

                color = TFT_BLUE;

            else if (level < 0.92f)

                color = TFT_MAGENTA;

            else

                color = TFT_WHITE;

            int y =

                BOTTOM -

                (s + 1) *

                STEP;

            tft.fillRect(

                x,

                y,

                width,

                LED_H,

                color

            );

        }

        previousNeon[i] =

            active;

    }

}

// ==================================================

// CENTER

// ==================================================

void drawCenterBars()

{

    constexpr int CY = 120;

    constexpr int MAX_H = 116;

    int barWidth =

        320 /

        BAND_COUNT;

    for (int i = 0;

         i < BAND_COUNT;

         i++)

    {

        float n =

            constrain(

                smoothedBands[i] /

                autoPeak,

                0.0f,

                1.0f

            );

        n =

            powf(

                n,

                2.0f

            );

        int height =

            n *

            MAX_H;

        int x =

            i * barWidth +

            1;

        int width =

            barWidth -

            3;

        if (

            height <

            previousCenter[i]

        )

        {

            int remove =

                previousCenter[i] -

                height;

            tft.fillRect(

                x,

                CY -

                previousCenter[i],

                width,

                remove,

                TFT_BLACK

            );

            tft.fillRect(

                x,

                CY +

                height,

                width,

                remove,

                TFT_BLACK

            );

        }

        uint16_t color =

            n < 0.5f

                ? TFT_CYAN

                : (

                    n < 0.8f

                        ? TFT_YELLOW

                        : TFT_RED

                );

        if (height > 0)

        {

            tft.fillRect(

                x,

                CY - height,

                width,

                height,

                color

            );

            tft.fillRect(

                x,

                CY,

                width,

                height,

                color

            );

        }

        previousCenter[i] =

            height;

    }

}

// ==================================================

// MIRROR

// ==================================================

void drawMirrorLed()

{

    constexpr int CY = 120;

    constexpr int STEP = 7;

    constexpr int LED_H = 5;

    constexpr int COUNT = 16;

    int barWidth =

        320 /

        BAND_COUNT;

    for (int i = 0;

         i < BAND_COUNT;

         i++)

    {

        float n =

            constrain(

                smoothedBands[i] /

                autoPeak,

                0.0f,

                1.0f

            );

        n =

            powf(

                n,

                2.2f

            );

        int active =

            constrain(

                int(

                    n * COUNT

                ),

                0,

                COUNT

            );

        int x =

            i * barWidth +

            1;

        int width =

            barWidth -

            3;

        if (

            active <

            previousMirror[i]

        )

        {

            for (

                int s = active;

                s <

                previousMirror[i];

                s++

            )

            {

                tft.fillRect(

                    x,

                    CY -

                    (s + 1) *

                    STEP,

                    width,

                    LED_H,

                    TFT_BLACK

                );

                tft.fillRect(

                    x,

                    CY +

                    s * STEP +

                    2,

                    width,

                    LED_H,

                    TFT_BLACK

                );

            }

        }

        for (int s = 0;

             s < active;

             s++)

        {

            float level =

                float(s + 1) /

                COUNT;

            uint16_t color =

                level < 0.55f

                    ? TFT_CYAN

                    : (

                        level < 0.82f

                            ? TFT_MAGENTA

                            : TFT_WHITE

                    );

            tft.fillRect(

                x,

                CY -

                (s + 1) *

                STEP,

                width,

                LED_H,

                color

            );

            tft.fillRect(

                x,

                CY +

                s * STEP +

                2,

                width,

                LED_H,

                color

            );

        }

        previousMirror[i] =

            active;

    }

}

// ==================================================

// PEAK HOLD

// ==================================================

void drawPeakHold()

{

    constexpr int BOTTOM = 238;

    constexpr int STEP = 8;

    constexpr int LED_H = 6;

    constexpr int COUNT = 29;

    int barWidth =

        320 /

        BAND_COUNT;

    for (int i = 0;

         i < BAND_COUNT;

         i++)

    {

        float n =

            constrain(

                smoothedBands[i] /

                autoPeak,

                0.0f,

                1.0f

            );

        n =

            powf(

                n,

                1.9f

            );

        int active =

            constrain(

                int(

                    n * COUNT

                ),

                0,

                COUNT

            );

        int x =

            i * barWidth +

            1;

        int width =

            barWidth -

            3;

        if (

            active >

            peakHoldPosition[i]

        )

        {

            peakHoldPosition[i] =

                active;

        }

        else

        {

            peakHoldPosition[i] -=

                0.10f;

            if (

                peakHoldPosition[i] <

                0

            )

            {

                peakHoldPosition[i] =

                    0;

            }

        }

        int newPeak =

            int(

                peakHoldPosition[i]

            );

        if (

            active <

            previousPeak[i]

        )

        {

            for (

                int s = active;

                s <

                previousPeak[i];

                s++

            )

            {

                if (

                    s + 1 ==

                    newPeak

                )

                {

                    continue;

                }

                tft.fillRect(

                    x,

                    BOTTOM -

                    (s + 1) *

                    STEP,

                    width,

                    LED_H,

                    TFT_BLACK

                );

            }

        }

        for (int s = 0;

             s < active;

             s++)

        {

            float level =

                float(s + 1) /

                COUNT;

            uint16_t color =

                level < 0.68f

                    ? TFT_GREEN

                    : (

                        level < 0.88f

                            ? TFT_ORANGE

                            : TFT_RED

                    );

            tft.fillRect(

                x,

                BOTTOM -

                (s + 1) *

                STEP,

                width,

                LED_H,

                color

            );

        }

        int oldPeak =

            previousPeakMarker[i];

        if (

            oldPeak > 0 &&

            oldPeak != newPeak

        )

        {

            int y =

                BOTTOM -

                oldPeak *

                STEP;

            if (

                oldPeak <= active

            )

            {

                float level =

                    float(oldPeak) /

                    COUNT;

                uint16_t color =

                    level < 0.68f

                        ? TFT_GREEN

                        : (

                            level < 0.88f

                                ? TFT_ORANGE

                                : TFT_RED

                        );

                tft.fillRect(

                    x,

                    y,

                    width,

                    3,

                    color

                );

            }

            else

            {

                tft.fillRect(

                    x,

                    y,

                    width,

                    3,

                    TFT_BLACK

                );

            }

        }

        if (newPeak > 0)

        {

            tft.fillRect(

                x,

                BOTTOM -

                newPeak *

                STEP,

                width,

                3,

                TFT_WHITE

            );

        }

        previousPeak[i] =

            active;

        previousPeakMarker[i] =

            newPeak;

    }

}

// ==================================================

// RAINBOW

// ==================================================

void drawRainbowLed()

{

    constexpr int BOTTOM = 238;

    constexpr int STEP = 8;

    constexpr int LED_H = 6;

    constexpr int COUNT = 29;

    int barWidth =

        320 /

        BAND_COUNT;

    for (int i = 0;

         i < BAND_COUNT;

         i++)

    {

        float n =

            constrain(

                smoothedBands[i] /

                autoPeak,

                0.0f,

                1.0f

            );

        n =

            powf(

                n,

                2.35f

            );

        int active =

            constrain(

                int(

                    n * COUNT

                ),

                0,

                COUNT

            );

        int x =

            i * barWidth +

            1;

        int width =

            barWidth -

            3;

        clearDroppedSegments(

            previousRainbow,

            i,

            active,

            x,

            width,

            BOTTOM,

            STEP,

            LED_H

        );

        uint8_t p =

            map(

                i,

                0,

                BAND_COUNT - 1,

                0,

                255

            );

        uint16_t color =

            rainbowColor(p);

        for (int s = 0;

             s < active;

             s++)

        {

            tft.fillRect(

                x,

                BOTTOM -

                (s + 1) *

                STEP,

                width,

                LED_H,

                color

            );

        }

        previousRainbow[i] =

            active;

    }

}

// ==================================================

// RETRO GREEN

// ==================================================

void drawRetroGreen()

{

    constexpr int BOTTOM = 238;

    constexpr int STEP = 10;

    constexpr int LED_H = 7;

    constexpr int COUNT = 23;

    int barWidth =

        320 /

        BAND12_COUNT;

    for (int i = 0;

         i < BAND12_COUNT;

         i++)

    {

        int active =

            constrain(

                int(

                    get12Band(

                        i,

                        1.75f

                    ) *

                    COUNT

                ),

                0,

                COUNT

            );

        int x =

            i * barWidth +

            2;

        int width =

            barWidth -

            5;

        clearDroppedSegments(

            previousRetro,

            i,

            active,

            x,

            width,

            BOTTOM,

            STEP,

            LED_H

        );

        for (int s = 0;

             s < active;

             s++)

        {

            tft.fillRect(

                x,

                BOTTOM -

                (s + 1) *

                STEP,

                width,

                LED_H,

                TFT_GREEN

            );

        }

        previousRetro[i] =

            active;

    }

}

// ==================================================

// VERTICAL RAINBOW

// ==================================================

void drawVerticalRainbow()

{

    constexpr int BOTTOM = 238;

    constexpr int STEP = 9;

    constexpr int LED_H = 7;

    constexpr int COUNT = 26;

    int barWidth =

        320 /

        BAND12_COUNT;

    for (int i = 0;

         i < BAND12_COUNT;

         i++)

    {

        int active =

            constrain(

                int(

                    get12Band(

                        i,

                        2.05f

                    ) *

                    COUNT

                ),

                0,

                COUNT

            );

        int x =

            i * barWidth +

            2;

        int width =

            barWidth -

            4;

        clearDroppedSegments(

            previousVertical,

            i,

            active,

            x,

            width,

            BOTTOM,

            STEP,

            LED_H

        );

        for (int s = 0;

             s < active;

             s++)

        {

            float level =

                float(s + 1) /

                COUNT;

            tft.fillRect(

                x,

                BOTTOM -

                (s + 1) *

                STEP,

                width,

                LED_H,

                verticalColor(

                    level

                )

            );

        }

        previousVertical[i] =

            active;

    }

}

// ==================================================

// COLOR WAVE

// ==================================================

void drawColorWave()

{

    constexpr int BOTTOM = 238;

    constexpr int STEP = 8;

    constexpr int LED_H = 6;

    constexpr int COUNT = 29;

    int barWidth =

        320 /

        BAND12_COUNT;

    uint8_t offset =

        (

            millis() /

            30

        ) &

        0xFF;

    for (int i = 0;

         i < BAND12_COUNT;

         i++)

    {

        int active =

            constrain(

                int(

                    get12Band(

                        i,

                        2.2f

                    ) *

                    COUNT

                ),

                0,

                COUNT

            );

        int x =

            i * barWidth +

            2;

        int width =

            barWidth -

            4;

        clearDroppedSegments(

            previousWave,

            i,

            active,

            x,

            width,

            BOTTOM,

            STEP,

            LED_H

        );

        uint16_t color =

            rainbowColor(

                offset +

                i * 21

            );

        for (int s = 0;

             s < active;

             s++)

        {

            tft.fillRect(

                x,

                BOTTOM -

                (s + 1) *

                STEP,

                width,

                LED_H,

                color

            );

        }

        previousWave[i] =

            active;

    }

}

// ==================================================

// PIXEL MATRIX

// ==================================================

uint16_t pixelInactiveColor()

{

    return tft.color565(

        0,

        12,

        8

    );

}

void preparePixelMatrix()

{

    constexpr int BOTTOM = 236;

    constexpr int STEP = 11;

    constexpr int LED_H = 8;

    constexpr int COUNT = 21;

    int barWidth =

        320 /

        BAND12_COUNT;

    for (int i = 0;

         i < BAND12_COUNT;

         i++)

    {

        int x =

            i * barWidth +

            2;

        int width =

            barWidth -

            5;

        for (int s = 0;

             s < COUNT;

             s++)

        {

            tft.fillRect(

                x,

                BOTTOM -

                (s + 1) *

                STEP,

                width,

                LED_H,

                pixelInactiveColor()

            );

        }

    }

}

void drawPixelMatrix()

{

    constexpr int BOTTOM = 236;

    constexpr int STEP = 11;

    constexpr int LED_H = 8;

    constexpr int COUNT = 21;

    int barWidth =

        320 /

        BAND12_COUNT;

    for (int i = 0;

         i < BAND12_COUNT;

         i++)

    {

        int active =

            constrain(

                int(

                    get12Band(

                        i,

                        1.95f

                    ) *

                    COUNT

                ),

                0,

                COUNT

            );

        int x =

            i * barWidth +

            2;

        int width =

            barWidth -

            5;

        if (

            active >

            previousMatrix[i]

        )

        {

            for (

                int s =

                    previousMatrix[i];

                s < active;

                s++

            )

            {

                tft.fillRect(

                    x,

                    BOTTOM -

                    (s + 1) *

                    STEP,

                    width,

                    LED_H,

                    verticalColor(

                        float(s + 1) /

                        COUNT

                    )

                );

            }

        }

        else if (

            active <

            previousMatrix[i]

        )

        {

            for (

                int s = active;

                s <

                previousMatrix[i];

                s++

            )

            {

                tft.fillRect(

                    x,

                    BOTTOM -

                    (s + 1) *

                    STEP,

                    width,

                    LED_H,

                    pixelInactiveColor()

                );

            }

        }

        previousMatrix[i] =

            active;

    }

}

// ==================================================

// FADE HELPERS

// ==================================================

uint16_t scaleColor565(
    uint16_t color,
    uint8_t brightness
)
{
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;

    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;

    return (r << 11) | (g << 5) | b;
}

constexpr uint8_t SCREEN_FADE_STEPS = 2;
constexpr uint16_t SCREEN_FADE_STEP_MS = 15;

// ==================================================

// EQ SCREEN

// ==================================================

void drawEqBand(

    uint8_t i,

    uint8_t brightness = 255

)

{

    int cx =

        eqCenters[i];

    bool selected =

        (

            uiState ==

                EQ_EDIT_MODE &&

            selectedEqBand ==

                i

        );

    tft.fillRect(

        cx - 27,

        EQ_TOP - 3,

        54,

        208,

        TFT_BLACK

    );

    tft.drawFastHLine(

        cx - 25,

        EQ_CENTER_Y,

        50,

        scaleColor565(
            tft.color565(
                50,
                50,
                50
            ),
            brightness
        )

    );

    uint16_t frameColor =

        scaleColor565(
            selected
                ? TFT_WHITE
                : tft.color565(
                    55,
                    55,
                    55
                ),
            brightness
        );

    tft.drawRect(

        cx -

            EQ_COLUMN_WIDTH / 2,

        EQ_TOP,

        EQ_COLUMN_WIDTH,

        EQ_BOTTOM -

            EQ_TOP,

        frameColor

    );

    float gain =

        constrain(

            eqGain[i],

            -12.0f,

            12.0f

        );

    int pixels =

        abs(gain) /

        12.0f *

        EQ_RANGE;

    uint16_t gainColor;

    if (gain > 0.0f)

    {

        gainColor =

            selected

                ? TFT_ORANGE

                : TFT_GREEN;

    }

    else if (

        gain < 0.0f

    )

    {

        gainColor =

            selected

                ? TFT_CYAN

                : TFT_BLUE;

    }

    else

    {

        gainColor =

            selected

                ? TFT_WHITE

                : tft.color565(

                    90,

                    90,

                    90

                );

    }

    gainColor =
        scaleColor565(
            gainColor,
            brightness
        );

    if (gain > 0)

    {

        tft.fillRect(

            cx -

                EQ_COLUMN_WIDTH / 2 +

                4,

            EQ_CENTER_Y -

                pixels,

            EQ_COLUMN_WIDTH -

                8,

            pixels,

            gainColor

        );

    }

    else if (gain < 0)

    {

        tft.fillRect(

            cx -

                EQ_COLUMN_WIDTH / 2 +

                4,

            EQ_CENTER_Y,

            EQ_COLUMN_WIDTH -

                8,

            pixels,

            gainColor

        );

    }

    else

    {

        tft.fillRect(

            cx -

                EQ_COLUMN_WIDTH / 2 +

                4,

            EQ_CENTER_Y -

                2,

            EQ_COLUMN_WIDTH -

                8,

            4,

            gainColor

        );

    }

    tft.setTextDatum(

        TC_DATUM

    );

    tft.setTextColor(

        scaleColor565(
            selected
                ? TFT_WHITE
                : TFT_CYAN,
            brightness
        ),

        TFT_BLACK

    );

    tft.drawString(

        eqBandNames[i],

        cx,

        202,

        2

    );

    char text[10];

    snprintf(

        text,

        sizeof(text),

        "%+.0f",

        eqGain[i]

    );

    tft.setTextColor(

        scaleColor565(
            selected
                ? TFT_YELLOW
                : tft.color565(
                    130,
                    130,
                    130
                ),
            brightness
        ),

        TFT_BLACK

    );

    tft.drawString(

        text,

        cx,

        221,

        2

    );

}

void drawEqualizerFull(
    uint8_t brightness = 255
)

{

    tft.fillScreen(

        TFT_BLACK

    );

    tft.drawFastHLine(

        10,

        EQ_CENTER_Y,

        300,

        scaleColor565(
            tft.color565(
                50,
                50,
                50
            ),
            brightness
        )

    );

    for (uint8_t i = 0;

         i < EQ_BAND_COUNT;

         i++)

    {

        drawEqBand(
            i,
            brightness
        );

    }

}

void fadeEqualizerIn()
{
    for (uint8_t step = 0;
         step <= SCREEN_FADE_STEPS;
         step++)
    {
        uint8_t brightness =
            (uint16_t(step) * 255) /
            SCREEN_FADE_STEPS;

        drawEqualizerFull(brightness);
        delay(SCREEN_FADE_STEP_MS);
    }
}

void fadeEqualizerOut()
{
    for (int step = SCREEN_FADE_STEPS;
         step >= 0;
         step--)
    {
        uint8_t brightness =
            (uint16_t(step) * 255) /
            SCREEN_FADE_STEPS;

        drawEqualizerFull(brightness);
        delay(SCREEN_FADE_STEP_MS);
    }

    tft.fillScreen(TFT_BLACK);
}

// ==================================================

// OVERLAY

// ==================================================

void showOverlay(

    const char* text

)

{

    overlayText =

        text;

    overlayTextLine2 =

        nullptr;

    overlayStart =

        millis();

    lastOverlayDraw =

        0;

}

void showOverlayTwoLines(

    const char* line1,

    const char* line2

)

{

    overlayText =

        line1;

    overlayTextLine2 =

        line2;

    overlayStart =

        millis();

    lastOverlayDraw =

        0;

}

void drawOverlay()

{

    if (

        overlayText ==

        nullptr

    )

    {

        return;

    }

    unsigned long now =

        millis();

    if (

        now -

        lastOverlayDraw <

        OVERLAY_REFRESH

    )

    {

        return;

    }

    lastOverlayDraw =

        now;

    unsigned long elapsed =

        now -

        overlayStart;

    bool twoLines =

        overlayTextLine2 !=

        nullptr;

    int overlayHeight =

        twoLines ?

            45 :

            26;

    if (

        elapsed >=

        OVERLAY_DURATION

    )

    {

        tft.fillRect(

            0,

            0,

            320,

            overlayTextLine2 != nullptr ? 45 : 26,

            TFT_BLACK

        );

        overlayText =

            nullptr;

        overlayTextLine2 =

            nullptr;

        // EQ is a static screen, so restore the part covered
        // by the notification after it disappears.
        if (

            visualMode ==

            EQUALIZER &&

            signalScreenState ==

            SCREEN_RUNNING

        )

        {

            drawEqualizerFull();

        }

        return;

    }

    tft.fillRect(

        0,

        0,

        320,

        overlayHeight,

        TFT_BLACK

    );

    float brightness = 1.0f;

    constexpr unsigned long FADE_IN_MS = 150;
    constexpr unsigned long HOLD_UNTIL_MS = 900;

    if (elapsed < FADE_IN_MS)

    {

        brightness =

            float(elapsed) /

            float(FADE_IN_MS);

    }

    else if (elapsed > HOLD_UNTIL_MS)

    {

        brightness =

            1.0f -

            float(

                elapsed -

                HOLD_UNTIL_MS

            ) /

            float(

                OVERLAY_DURATION -

                HOLD_UNTIL_MS

            );

    }

    brightness =

        constrain(

            brightness,

            0.0f,

            1.0f

        );

    uint8_t c =

        255.0f *

        brightness;

    tft.setTextDatum(

        TC_DATUM

    );

    tft.setTextColor(

        tft.color565(

            c,

            c,

            c

        ),

        TFT_BLACK

    );

    if (twoLines)

    {

        tft.drawString(

            overlayText,

            160,

            2,

            2

        );

        tft.drawString(

            overlayTextLine2,

            160,

            22,

            2

        );

    }

    else

    {

        tft.drawString(

            overlayText,

            160,

            4,

            2

        );

    }

}

void showVisualModeOverlay()
{
    showOverlay(
        visualModeNames[
            static_cast<uint8_t>(
                visualMode
            )
        ]
    );
}

// ==================================================

// RESET VISUAL STATE

// ==================================================

void resetVisualState()

{

    memset(

        previousClassic,

        0,

        sizeof(previousClassic)

    );

    memset(

        previousNeon,

        0,

        sizeof(previousNeon)

    );

    memset(

        previousCenter,

        0,

        sizeof(previousCenter)

    );

    memset(

        previousMirror,

        0,

        sizeof(previousMirror)

    );

    memset(

        previousPeak,

        0,

        sizeof(previousPeak)

    );

    memset(

        previousRainbow,

        0,

        sizeof(previousRainbow)

    );

    memset(

        previousPeakMarker,

        0,

        sizeof(previousPeakMarker)

    );

    memset(

        peakHoldPosition,

        0,

        sizeof(peakHoldPosition)

    );

    memset(

        previousRetro,

        0,

        sizeof(previousRetro)

    );

    memset(

        previousVertical,

        0,

        sizeof(previousVertical)

    );

    memset(

        previousWave,

        0,

        sizeof(previousWave)

    );

    memset(

        previousMatrix,

        0,

        sizeof(previousMatrix)

    );

    memset(

        smoothedBands,

        0,

        sizeof(smoothedBands)

    );

    autoPeak =

        1.0f;

}

// ==================================================

// CHANGE VISUAL MODE

// ==================================================

void changeVisualMode(

    int direction

)

{

    if (

        uiState ==

        EQ_EDIT_MODE

    )

    {

        if (!audioProcessingEnabled)
        {
            uiState =
                PLAYBACK_MODE;

            drawEqualizerFull();

            showOverlayTwoLines(
                "Audio Processing OFF.",
                "Switch ON to continue."
            );

            return;
        }

        eqGain[selectedEqBand] +=

            direction;

        eqGain[selectedEqBand] =

            constrain(

                eqGain[selectedEqBand],

                -12.0f,

                12.0f

            );

        updateEQCoefficients();

        drawEqBand(

            selectedEqBand

        );

        Serial.printf(

            "EQ %s Hz: %+.0f dB\n",

            eqBandNames[

                selectedEqBand

            ],

            eqGain[

                selectedEqBand

            ]

        );

        return;

    }

    VisualMode oldMode = visualMode;

    int mode =

        int(visualMode) +

        direction;

    if (mode >=

        MODE_COUNT)

    {

        mode =

            0;

    }

    if (mode < 0)

    {

        mode =

            MODE_COUNT -

            1;

    }

    if (oldMode == EQUALIZER)
    {
        fadeEqualizerOut();
    }

    visualMode =

        static_cast<

            VisualMode

        >(mode);

    resetVisualState();

    tft.fillScreen(

        TFT_BLACK

    );

    if (

        visualMode ==

        PIXEL_MATRIX

    )

    {

        preparePixelMatrix();

    }

    else if (

        visualMode ==

        EQUALIZER

    )

    {

        fadeEqualizerIn();

    }

    showVisualModeOverlay();

}

// ==================================================

// ENCODER

// ==================================================

void updateEncoder()

{

    int8_t step =

        0;

    noInterrupts();

    if (

        encoderPendingSteps >

        0

    )

    {

        encoderPendingSteps = encoderPendingSteps - 1;

        step = +1;

    }

    else if (

        encoderPendingSteps <

        0

    )

    {

        encoderPendingSteps = encoderPendingSteps + 1;

        step = -1;

    }

    interrupts();

    if (step != 0)

    {

        changeVisualMode(

            step

        );

    }

}

// ==================================================

// BUTTON

// ==================================================

void handleButtonPress()

{

    if (

        visualMode !=

        EQUALIZER

    )

    {

        return;

    }

    if (

        uiState ==

        PLAYBACK_MODE

    )

    {

        if (!audioProcessingEnabled)
        {
            showOverlayTwoLines(
                "Audio Processing OFF.",
                "Switch ON to continue."
            );

            return;
        }

        uiState =

            EQ_EDIT_MODE;

        selectedEqBand =

            0;

        drawEqBand(

            0

        );

        showOverlay(

            "EDIT MODE"

        );

        return;

    }

    uint8_t oldBand =

        selectedEqBand;

    selectedEqBand++;

    if (

        selectedEqBand >=

        EQ_BAND_COUNT

    )

    {

        selectedEqBand =

            0;

        uiState =

            PLAYBACK_MODE;

        drawEqBand(

            oldBand

        );

        showOverlay(

            "PLAYBACK MODE"

        );

        return;

    }

    drawEqBand(

        oldBand

    );

    drawEqBand(

        selectedEqBand

    );

}

void updateButton()

{

    bool current =

        digitalRead(

            ENC_BTN

        );

    if (

        current !=

        lastButtonState

    )

    {

        lastButtonState =

            current;

        lastButtonChange =

            millis();

    }

    if (

        millis() -

        lastButtonChange >

        BUTTON_DEBOUNCE_MS

    )

    {

        if (

            current !=

            stableButtonState

        )

        {

            stableButtonState =

                current;

            if (

                stableButtonState ==

                LOW

            )

            {

                handleButtonPress();

            }

        }

    }

}

// ==================================================
// KEY0 - AUDIO PROCESSING ON/OFF
// ==================================================

void setAudioProcessing(

    bool enabled

)

{

    if (

        audioProcessingEnabled ==

        enabled

    )

    {

        return;

    }

    audioProcessingEnabled =

        enabled;

    // Editing is never allowed while bypass is active.
    if (
        !enabled &&
        uiState == EQ_EDIT_MODE
    )
    {
        uiState =
            PLAYBACK_MODE;

        if (
            visualMode ==
            EQUALIZER
        )
        {
            drawEqualizerFull();
        }
    }

    if (enabled)
    {
        showOverlay(
            "Audio Processing ON"
        );

        Serial.println(
            "Audio Processing ON"
        );
    }
    else
    {
        showOverlay(
            "Audio Processing OFF"
        );

        Serial.println(
            "Audio Processing OFF - true PCM bypass"
        );
    }

}

void updateKey0()

{

    bool current =

        digitalRead(

            KEY0_PIN

        );

    if (

        current !=

        lastKey0State

    )

    {

        lastKey0State =

            current;

        lastKey0Change =

            millis();

    }

    if (

        millis() -

        lastKey0Change >

        KEY0_DEBOUNCE_MS

    )

    {

        if (

            current !=

            stableKey0State

        )

        {

            stableKey0State =

                current;

            if (

                stableKey0State ==

                LOW

            )

            {

                setAudioProcessing(

                    !audioProcessingEnabled

                );

            }

        }

    }

}

// ==================================================

// DRAW VISUALIZATION

// ==================================================

void drawSpectrum()

{

    switch (visualMode)

    {

        case CLASSIC_LED:

            drawClassicLed();

            break;

        case NEON_DYNAMIC:

            drawNeonDynamic();

            break;

        case CENTER_BARS:

            drawCenterBars();

            break;

        case MIRROR_LED:

            drawMirrorLed();

            break;

        case PEAK_HOLD:

            drawPeakHold();

            break;

        case RAINBOW_LED:

            drawRainbowLed();

            break;

        case RETRO_GREEN:

            drawRetroGreen();

            break;

        case VERTICAL_RAINBOW:

            drawVerticalRainbow();

            break;

        case COLOR_WAVE:

            drawColorWave();

            break;

        case PIXEL_MATRIX:

            drawPixelMatrix();

            break;

        case EQUALIZER:

            // EQ redraw only on user interaction

            break;

        default:

            break;

    }

}

// ==================================================

// STARTUP / SIGNAL SCREENS

// ==================================================

void drawCenteredMessage(
    const char* message,
    uint8_t brightness
)
{
    tft.fillScreen(TFT_BLACK);

    tft.setTextDatum(MC_DATUM);

    uint16_t color =
        tft.color565(
            brightness,
            brightness,
            brightness
        );

    tft.setTextColor(
        color,
        TFT_BLACK
    );

    tft.drawString(
        message,
        160,
        120,
        4
    );
}

void fadeMessageIn(
    const char* message
)
{
    for (uint8_t step = 0;
         step <= SCREEN_FADE_STEPS;
         step++)
    {
        uint8_t brightness =
            (uint16_t(step) * 255) /
            SCREEN_FADE_STEPS;

        drawCenteredMessage(
            message,
            brightness
        );

        delay(SCREEN_FADE_STEP_MS);
    }
}

void fadeMessageOut(
    const char* message
)
{
    for (int step = SCREEN_FADE_STEPS;
         step >= 0;
         step--)
    {
        uint8_t brightness =
            (uint16_t(step) * 255) /
            SCREEN_FADE_STEPS;

        drawCenteredMessage(
            message,
            brightness
        );

        delay(SCREEN_FADE_STEP_MS);
    }

    tft.fillScreen(TFT_BLACK);
}

void transitionMessage(
    const char* from,
    const char* to
)
{
    fadeMessageOut(from);
    fadeMessageIn(to);
}

void enterWaitingBluetoothScreen()
{
    if (signalScreenState == SCREEN_RUNNING)
    {
        if (visualMode == EQUALIZER)
        {
            fadeEqualizerOut();
        }
        else
        {
            tft.fillScreen(TFT_BLACK);
        }

        fadeMessageIn(TEXT_WAITING_BT);
    }
    else if (signalScreenState == SCREEN_WAITING_AUDIO)
    {
        transitionMessage(
            TEXT_WAITING_AUDIO,
            TEXT_WAITING_BT
        );
    }
    else
    {
        fadeMessageIn(TEXT_WAITING_BT);
    }

    signalScreenState = SCREEN_WAITING_BT;
    waitingForAudio = true;
    fftReady = false;
    fftIndex = 0;
}

void enterWaitingAudioScreen()
{
    if (signalScreenState == SCREEN_RUNNING)
    {
        if (visualMode == EQUALIZER)
        {
            fadeEqualizerOut();
        }
        else
        {
            tft.fillScreen(TFT_BLACK);
        }

        fadeMessageIn(TEXT_WAITING_AUDIO);
    }
    else if (signalScreenState == SCREEN_WAITING_BT)
    {
        transitionMessage(
            TEXT_WAITING_BT,
            TEXT_WAITING_AUDIO
        );
    }

    signalScreenState = SCREEN_WAITING_AUDIO;
    waitingForAudio = true;
    fftReady = false;
    fftIndex = 0;
}

void enterRunningScreen()
{
    if (signalScreenState == SCREEN_WAITING_AUDIO)
    {
        fadeMessageOut(TEXT_WAITING_AUDIO);
    }
    else if (signalScreenState == SCREEN_WAITING_BT)
    {
        fadeMessageOut(TEXT_WAITING_BT);
    }

    tft.fillScreen(TFT_BLACK);
    resetVisualState();

    signalScreenState = SCREEN_RUNNING;
    waitingForAudio = false;

    if (visualMode == PIXEL_MATRIX)
    {
        preparePixelMatrix();
    }
    else if (visualMode == EQUALIZER)
    {
        fadeEqualizerIn();
    }

    showVisualModeOverlay();
}

void updateSignalScreenState()
{
    if (btConnectionChanged)
    {
        bool connected = btConnected;
        btConnectionChanged = false;

        if (connected)
        {
            enterWaitingAudioScreen();
            Serial.println("BT connected - waiting for audio signal");
        }
        else
        {
            enterWaitingBluetoothScreen();
            Serial.println("BT disconnected");
        }
    }

    if (
        signalScreenState == SCREEN_RUNNING &&
        btConnected &&
        (millis() - lastAudioSignalMs) >
            AUDIO_SIGNAL_TIMEOUT_MS
    )
    {
        enterWaitingAudioScreen();
        Serial.println("Audio signal lost");
    }
}

// ==================================================

// SETUP

// ==================================================

void setup()

{

    Serial.begin(

        115200

    );

    delay(

        1000

    );

    // ----------------------------------------------

    // ENCODER

    // ----------------------------------------------

    pinMode(

        ENC_A,

        INPUT_PULLUP

    );

    pinMode(

        ENC_B,

        INPUT_PULLUP

    );

    pinMode(

        ENC_BTN,

        INPUT_PULLUP

    );

    pinMode(

        KEY0_PIN,

        INPUT_PULLUP

    );

    lastKey0State =
        digitalRead(
            KEY0_PIN
        );

    stableKey0State =
        lastKey0State;

    encoderState =

        (

            digitalRead(

                ENC_A

            ) <<

            1

        ) |

        digitalRead(

            ENC_B

        );

    attachInterrupt(

        digitalPinToInterrupt(

            ENC_A

        ),

        encoderISR,

        CHANGE

    );

    attachInterrupt(

        digitalPinToInterrupt(

            ENC_B

        ),

        encoderISR,

        CHANGE

    );

    // ----------------------------------------------

    // I2S

    // ----------------------------------------------

    i2s.setPins(

        I2S_BCLK,

        I2S_LRCK,

        I2S_DATA

    );

    if (

        !i2s.begin(

            I2S_MODE_STD,

            44100,

            I2S_DATA_BIT_WIDTH_16BIT,

            I2S_SLOT_MODE_STEREO,

            I2S_STD_SLOT_BOTH

        )

    )

    {

        Serial.println(

            "I2S initialization failed"

        );

        while (true)

        {

            delay(

                1000

            );

        }

    }

    // ----------------------------------------------

    // TFT

    // ----------------------------------------------

    tft.init();

    tft.setRotation(

        3

    );

    fadeMessageIn(
        TEXT_WAITING_BT
    );

    signalScreenState =
        SCREEN_WAITING_BT;

    // ----------------------------------------------

    // EQ

    // ----------------------------------------------

    updateEQCoefficients();

    // ----------------------------------------------

    // BLUETOOTH

    // ----------------------------------------------

    a2dpSink.set_on_connection_state_changed(
        btConnectionStateChanged
    );

    a2dpSink.set_stream_reader(

        audioCallback,

        false

    );

    a2dpSink.start(

        "Marocco's BT Audio Receiver"

    );

    Serial.println(

        "Bluetooth A2DP + DSP EQ started"

    );

    Serial.println(

        "Waiting for BT audio..."

    );

}

// ==================================================

// LOOP

// ==================================================

void loop()
{
    updateSignalScreenState();
    updateKey0();

    if (signalScreenState == SCREEN_RUNNING)
    {
        updateEncoder();
        updateButton();
    }

    // ----------------------------------------------
    // FIRST / NEXT AUDIO FRAME
    // ----------------------------------------------

    if (fftReady)
    {
        if (
            waitingForAudio &&
            btConnected &&
            lastAudioSignalMs > 0 &&
            (millis() - lastAudioSignalMs) < 250
        )
        {
            enterRunningScreen();

            Serial.println(
                "BT audio received"
            );
        }

        // ------------------------------------------
        // FFT
        // ------------------------------------------

        processFFT();

        if (
            signalScreenState == SCREEN_RUNNING &&
            visualMode != EQUALIZER
        )
        {
            drawSpectrum();
        }

        fftReady = false;
    }

    // ----------------------------------------------
    // OVERLAY
    // ----------------------------------------------

    if (
        signalScreenState == SCREEN_RUNNING
    )
    {
        drawOverlay();
    }

    delay(1);
}