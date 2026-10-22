#include "Equalizer.h"
#include <Preferences.h>
#include <cmath>

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

BiquadCoeffs eqCoeffBank[2][EQ_BAND_COUNT];
bool eqActiveBank[2][EQ_BAND_COUNT];
float eqPreampBank[2] = {1.0f, 1.0f};
volatile uint8_t activeEqBank = 0;
BiquadState eqStateL[EQ_BAND_COUNT];
BiquadState eqStateR[EQ_BAND_COUNT];

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

    // SMART BOOST: filters apply the requested gain without continuously
    // attenuating every frequency. Peak protection is performed after the
    // EQ, on the actual stereo sample pair in AudioEngine.cpp.
    for (uint8_t i = 0; i < EQ_BAND_COUNT; ++i)
    {
        eqActiveBank[newBank][i] = fabsf(eqGain[i]) > 0.05f;
    }

    eqPreampBank[newBank] = 1.0f;

    activeEqBank =

        newBank;

}


// ==================================================
// PERSISTENT EQ SETTINGS - NVS
// ==================================================
namespace {
constexpr uint32_t EQ_SAVE_DELAY_MS = 3000;
constexpr const char* EQ_NVS_NAMESPACE = "eq-settings";
constexpr const char* EQ_NVS_KEY = "gains-v1";
uint32_t eqLastChangeMs = 0;
bool eqSavePending = false;
float eqLastSaved[EQ_BAND_COUNT] = {};
bool eqHaveLastSaved = false;

bool eqSettingsDifferFromSaved()
{
    if (!eqHaveLastSaved) return true;
    for (uint8_t i = 0; i < EQ_BAND_COUNT; ++i)
        if (fabsf(eqGain[i] - eqLastSaved[i]) > 0.0001f) return true;
    return false;
}
} // namespace

void loadEQSettings()
{
    Preferences prefs;
    if (prefs.begin(EQ_NVS_NAMESPACE, true))
    {
        float saved[EQ_BAND_COUNT];
        if (prefs.getBytesLength(EQ_NVS_KEY) == sizeof(saved) &&
            prefs.getBytes(EQ_NVS_KEY, saved, sizeof(saved)) == sizeof(saved))
        {
            bool valid = true;
            for (uint8_t i = 0; i < EQ_BAND_COUNT; ++i)
                if (!std::isfinite(saved[i]) || saved[i] < -12.0f || saved[i] > 12.0f)
                    valid = false;
            if (valid)
            {
                for (uint8_t i = 0; i < EQ_BAND_COUNT; ++i) eqGain[i] = saved[i];
                Serial.println("[EQ NVS] Stored equalizer restored");
            }
            else Serial.println("[EQ NVS] Invalid settings; using defaults");
        }
        prefs.end();
    }
    for (uint8_t i = 0; i < EQ_BAND_COUNT; ++i) eqLastSaved[i] = eqGain[i];
    eqHaveLastSaved = true;
    eqSavePending = false;
}

void scheduleEQSave()
{
    // Called only when the user actually changes a band.
    eqLastChangeMs = millis();
    eqSavePending = true;
}

void serviceEQSave()
{
    if (!eqSavePending || millis() - eqLastChangeMs < EQ_SAVE_DELAY_MS) return;
    eqSavePending = false;
    if (!eqSettingsDifferFromSaved()) return;

    Preferences prefs;
    if (!prefs.begin(EQ_NVS_NAMESPACE, false))
    {
        Serial.println("[EQ NVS] Open failed; will retry");
        eqSavePending = true;
        eqLastChangeMs = millis();
        return;
    }
    const size_t written = prefs.putBytes(EQ_NVS_KEY, eqGain, sizeof(float) * EQ_BAND_COUNT);
    prefs.end();
    if (written == sizeof(float) * EQ_BAND_COUNT)
    {
        for (uint8_t i = 0; i < EQ_BAND_COUNT; ++i) eqLastSaved[i] = eqGain[i];
        eqHaveLastSaved = true;
        Serial.println("[EQ NVS] Saved 5 bands");
    }
    else
    {
        Serial.println("[EQ NVS] Save failed; will retry");
        eqSavePending = true;
        eqLastChangeMs = millis();
    }
}
