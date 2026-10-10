#include "Equalizer.h"

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

