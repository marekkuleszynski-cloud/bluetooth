#include "DisplayManager.h"
#include <cstring>
#include "AudioEngine.h"
#include <math.h>

TFT_eSPI tft;

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
    "Dual Gauge",
    "Retro VU",
    "Winamp Classic",
    "Winamp Fire",
    "Scope CRT",
    "Scope Stereo",
    "Equalizer"
};

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

    48,

    104,

    160,

    216,

    272

};

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

    constexpr int COUNT = 23;

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

            2;

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

    constexpr int MAX_H = 88;

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

            2;

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

    constexpr int COUNT = 12;

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

            2;

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

// Peak Hold dynamics:
// - one white peak marker per band
// - the old marker is always erased/restored before the new one is drawn
// - bars react quickly and the peak falls faster than in the original
void drawPeakHold()
{
    constexpr int BOTTOM = 238;
    constexpr int STEP = 8;
    constexpr int LED_H = 6;
    constexpr int COUNT = 25;
    constexpr float PEAK_FALL_PER_FRAME = 0.42f;
    constexpr float BAR_EXPONENT = 1.55f;

    const int barWidth = 320 / BAND_COUNT;

    for (int i = 0; i < BAND_COUNT; ++i)
    {
        // Keep smoothing for stability, but respond more quickly to transients.
        float level = 0.70f * bands[i] +
                      0.30f * smoothedBands[i];

        float n = constrain(level / autoPeak, 0.0f, 1.0f);
        n = powf(n, BAR_EXPONENT);

        const int active = constrain(
            static_cast<int>(n * COUNT), 0, COUNT
        );

        const int x = i * barWidth + 1;
        const int width = barWidth - 2;

        // Single falling marker for each frequency band.
        if (active >= peakHoldPosition[i])
        {
            peakHoldPosition[i] = static_cast<float>(active);
        }
        else
        {
            peakHoldPosition[i] -= PEAK_FALL_PER_FRAME;
            if (peakHoldPosition[i] < active)
                peakHoldPosition[i] = static_cast<float>(active);
        }

        const int newPeak = constrain(
            static_cast<int>(peakHoldPosition[i]), 0, COUNT
        );
        const int oldPeak = previousPeakMarker[i];

        // Erase the *whole* old white marker.
        // When the underlying LED remains active, restore its original color.
        // Otherwise remove the marker completely.
        if (oldPeak > 0 && oldPeak != newPeak)
        {
            const float oldLevel = float(oldPeak) / COUNT;
            const uint16_t ledColor =
                oldLevel < 0.68f ? TFT_GREEN :
                oldLevel < 0.88f ? TFT_ORANGE : TFT_RED;

            tft.fillRect(
                x,
                BOTTOM - oldPeak * STEP,
                width,
                LED_H,
                oldPeak <= active ? ledColor : TFT_BLACK
            );
        }

        // Fully clear deactivated LEDs, including any former marker.
        for (int seg = active; seg < previousPeak[i]; ++seg)
        {
            tft.fillRect(
                x, BOTTOM - (seg + 1) * STEP,
                width, LED_H, TFT_BLACK
            );
        }

        // Repaint active LEDs in their natural colors.
        for (int seg = 0; seg < active; ++seg)
        {
            const float ledLevel = float(seg + 1) / COUNT;
            const uint16_t color =
                ledLevel < 0.68f ? TFT_GREEN :
                ledLevel < 0.88f ? TFT_ORANGE : TFT_RED;

            tft.fillRect(
                x, BOTTOM - (seg + 1) * STEP,
                width, LED_H, color
            );
        }

        // Draw precisely one marker, always last.
        if (newPeak > 0)
        {
            tft.fillRect(
                x, BOTTOM - newPeak * STEP,
                width, 3, TFT_WHITE
            );
        }

        previousPeak[i] = active;
        previousPeakMarker[i] = newPeak;
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

    constexpr int COUNT = 25;

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

            2;

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

    constexpr int COUNT = 20;

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

        int x = (i * 320) / BAND12_COUNT + 2;
        int width = ((i + 1) * 320) / BAND12_COUNT -
                    (i * 320) / BAND12_COUNT - 4;

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

                        2.05f

                    ) *

                    COUNT

                ),

                0,

                COUNT

            );

        int x = (i * 320) / BAND12_COUNT + 2;
        int width = ((i + 1) * 320) / BAND12_COUNT -
                    (i * 320) / BAND12_COUNT - 4;

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

    constexpr int COUNT = 25;

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

        int x = (i * 320) / BAND12_COUNT + 2;
        int width = ((i + 1) * 320) / BAND12_COUNT -
                    (i * 320) / BAND12_COUNT - 4;

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

    constexpr int COUNT = 18;

    int barWidth =

        320 /

        BAND12_COUNT;

    for (int i = 0;

         i < BAND12_COUNT;

         i++)

    {

        int x = (i * 320) / BAND12_COUNT + 2;
        int width = ((i + 1) * 320) / BAND12_COUNT -
                    (i * 320) / BAND12_COUNT - 4;

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

    constexpr int COUNT = 18;

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

        int x = (i * 320) / BAND12_COUNT + 2;
        int width = ((i + 1) * 320) / BAND12_COUNT -
                    (i * 320) / BAND12_COUNT - 4;

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

// ==================================================
// EQUALIZER — MINIMAL MONOCHROME GAIN LINES
// ==================================================

// ==================================================
// EQUALIZER — METALLIC 3D, GREY TRACKS, RED HIGHLIGHTS
// ==================================================

// Retain the last rendered gain and selection state to avoid repainting
// the entire gradient track on each 1 dB encoder step.
static bool eqTrackCacheValid[EQ_BAND_COUNT] = {};
static float eqTrackCachedGain[EQ_BAND_COUNT] = {};
static bool eqTrackCachedSelected[EQ_BAND_COUNT] = {};

static int eqMarkerY(float gain)
{
    return EQ_CENTER_Y - static_cast<int>(
        roundf(constrain(gain, -12.0f, 12.0f) / 12.0f * (EQ_RANGE - 5))
    );
}

// Brushed metal fill: vertical illumination and side-to-side curvature.
// Used both for the first draw and when restoring pixels under the old marker.
static uint16_t eqMetalColor(int x, int y, int left, int top, int height)
{
    const int progress = ((y - top) * 100) / height;
    int v = 236 - progress / 3;
    if (progress > 53) v -= 17;
    const int dx = x - (left + EQ_COLUMN_WIDTH / 2);
    // Recessed rim at left; bright specular reflection slightly right of center.
    if (dx < -14) v -= 48;
    else if (dx < -10) v -= 24;
    else if (dx > 13) v -= 30;
    else if (dx > 10) v -= 11;
    else if (dx >= 4 && dx <= 6) v += 13;
    else if (dx < -3) v -= 6;
    v = constrain(v, 0, 255);
    return tft.color565(v, v, min(255, v + 2));
}

// Restore only the old marker's ~5 scanlines, including graduation marks.
static void restoreEqTrackRows(uint8_t i, int markerY)
{
    const int left = eqCenters[i] - EQ_COLUMN_WIDTH / 2;
    const int top = EQ_TOP;
    const int height = EQ_BOTTOM - EQ_TOP;
    for (int yy = markerY - 2; yy <= markerY + 2; ++yy)
    {
        if (yy < top + 2 || yy >= top + height - 2) continue;
        // Restore the exact curved metal background (not a flat grey strip).
        for (int xx = left + 4; xx < left + EQ_COLUMN_WIDTH - 4; ++xx)
            tft.drawPixel(xx, yy, eqMetalColor(xx, yy, left, top, height));
        for (int db = -9; db <= 9; db += 3)
        {
            if (yy == eqMarkerY(static_cast<float>(db)))
                tft.drawFastHLine(left + 11, yy,
                    EQ_COLUMN_WIDTH - 22, tft.color565(105, 106, 111));
        }
        if (yy == EQ_CENTER_Y)
            tft.drawFastHLine(left + 4, yy,
                EQ_COLUMN_WIDTH - 8, tft.color565(69, 70, 74));
    }
}

static void drawEqMarker(uint8_t i, uint8_t brightness)
{
    const int left = eqCenters[i] - EQ_COLUMN_WIDTH / 2;
    const int y = eqMarkerY(eqGain[i]);
    tft.drawFastHLine(left + 4, y - 2, EQ_COLUMN_WIDTH - 8,
        scaleColor565(tft.color565(190, 190, 194), brightness));
    tft.fillRect(left + 4, y - 1, EQ_COLUMN_WIDTH - 8, 3,
        scaleColor565(tft.color565(17, 17, 21), brightness));
    tft.drawFastHLine(left + 5, y, EQ_COLUMN_WIDTH - 10,
        scaleColor565(tft.color565(56, 56, 62), brightness));
    tft.drawFastHLine(left + 5, y + 2, EQ_COLUMN_WIDTH - 10,
        scaleColor565(tft.color565(231, 24, 27), brightness));
}

static void drawEqGainText(uint8_t i, uint8_t brightness)
{
    const int cx = eqCenters[i];
    const bool boosted = eqGain[i] > 0.05f;
    char gainText[12];
    snprintf(gainText, sizeof(gainText), "%+.0f", eqGain[i]);
    tft.fillRect(cx - 24, 220, 48, 18, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(
        scaleColor565(
            boosted ? tft.color565(244, 43, 45)
                    : tft.color565(185, 186, 191),
            brightness
        ), TFT_BLACK
    );
    tft.drawString(gainText, cx, 221, 2);
}

void drawEqBand(uint8_t i, uint8_t brightness)
{
    if (i >= EQ_BAND_COUNT) return;

    const int cx = eqCenters[i];
    const int left = cx - EQ_COLUMN_WIDTH / 2;
    const int top = EQ_TOP;
    const int height = EQ_BOTTOM - EQ_TOP;
    const bool selected = (uiState == EQ_EDIT_MODE && selectedEqBand == i);

    // Encoder gain updates: only the previous marker, new marker and dB
    // readout need refreshing. The metallic background remains untouched.
    if (brightness == 255 && eqTrackCacheValid[i] &&
        eqTrackCachedSelected[i] == selected)
    {
        if (eqTrackCachedGain[i] != eqGain[i])
        {
            restoreEqTrackRows(i, eqMarkerY(eqTrackCachedGain[i]));
            drawEqMarker(i, brightness);
            drawEqGainText(i, brightness);
            eqTrackCachedGain[i] = eqGain[i];
        }
        return;
    }

    // Selection change: touch ONLY the outside highlight frame.
    // Do not erase/rebuild the expensive 3D track.
    if (brightness == 255 && eqTrackCacheValid[i] &&
        eqTrackCachedGain[i] == eqGain[i] &&
        eqTrackCachedSelected[i] != selected)
    {
        const uint16_t rimHi = tft.color565(selected ? 246 : 168,
                                                 selected ? 246 : 170,
                                                 selected ? 248 : 176);
        // 3 px active outline: subtle red exterior, red center and bright bevel.
        // On deselection restore the original shadow + two metal rims.
        tft.drawRect(left - 3, top - 3,
                     EQ_COLUMN_WIDTH + 6, height + 6,
                     selected ? tft.color565(145, 19, 24)
                              : tft.color565(8, 8, 10));
        tft.drawRect(left - 2, top - 2,
                     EQ_COLUMN_WIDTH + 4, height + 4,
                     selected ? tft.color565(205, 28, 34)
                              : tft.color565(57, 58, 65));
        tft.drawRect(left - 1, top - 1,
                     EQ_COLUMN_WIDTH + 2, height + 2,
                     selected ? tft.color565(248, 108, 112) : rimHi);
        eqTrackCachedSelected[i] = selected;
        return;
    }

    // Full band redraw only on first entry (or deliberate full refresh).
    tft.fillRect(cx - 26, top - 3, 52, 208, TFT_BLACK);
    auto shade = [brightness](uint16_t color) -> uint16_t {
        return scaleColor565(color, brightness);
    };

    const uint16_t shadow = shade(tft.color565(8, 8, 10));
    const uint16_t rimHi = shade(tft.color565(selected ? 246 : 168,
                                                  selected ? 246 : 170,
                                                  selected ? 248 : 176));
    const uint16_t rimLo = shade(tft.color565(57, 58, 65));

    tft.fillRoundRect(left - 3, top - 3,
                      EQ_COLUMN_WIDTH + 6, height + 6, 3, shadow);
    // Same 3 px outline on initial render and on selection changes.
    tft.drawRect(left - 3, top - 3, EQ_COLUMN_WIDTH + 6,
                 height + 6,
                 selected ? shade(tft.color565(145, 19, 24)) : shadow);
    tft.drawRect(left - 2, top - 2, EQ_COLUMN_WIDTH + 4,
                 height + 4,
                 selected ? shade(tft.color565(205, 28, 34)) : rimLo);
    tft.drawRect(left - 1, top - 1, EQ_COLUMN_WIDTH + 2,
                 height + 2,
                 selected ? shade(tft.color565(248, 108, 112)) : rimHi);
    tft.drawRect(left, top, EQ_COLUMN_WIDTH, height, rimLo);
    // Double inner rim: dark recess on top/left, light bevel at bottom/right.
    tft.drawFastHLine(left + 1, top + 1, EQ_COLUMN_WIDTH - 2,
                      shade(tft.color565(49, 50, 56)));
    tft.drawFastHLine(left + 1, top + height - 2, EQ_COLUMN_WIDTH - 2,
                      shade(tft.color565(248, 248, 252)));
    tft.drawFastVLine(left + 1, top + 1, height - 2,
                      shade(tft.color565(42, 43, 49)));
    tft.drawFastVLine(left + EQ_COLUMN_WIDTH - 2, top + 1, height - 2,
                      shade(tft.color565(245, 245, 248)));

    for (int yy = top + 2; yy < top + height - 2; ++yy)
    {
        // Horizontal specular profile approximates a curved metal surface.
        for (int xx = left + 2; xx < left + EQ_COLUMN_WIDTH - 2; ++xx)
            tft.drawPixel(xx, yy,
                shade(eqMetalColor(xx, yy, left, top, height)));
    }

    for (int db = -9; db <= 9; db += 3)
    {
        const int yTick = eqMarkerY(static_cast<float>(db));
        tft.drawFastHLine(left + 11, yTick, EQ_COLUMN_WIDTH - 22,
                          shade(tft.color565(105, 106, 111)));
    }
    tft.drawFastHLine(left + 4, EQ_CENTER_Y, EQ_COLUMN_WIDTH - 8,
                      shade(tft.color565(69, 70, 74)));

    drawEqMarker(i, brightness);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(shade(tft.color565(209, 209, 213)), TFT_BLACK);
    tft.drawString(eqBandNames[i], cx, 201, 2);
    drawEqGainText(i, brightness);

    eqTrackCacheValid[i] = (brightness == 255);
    eqTrackCachedGain[i] = eqGain[i];
    eqTrackCachedSelected[i] = selected;
}

// ±12 dB graduation on BOTH sides, with red +12/0/-12 reference ticks.
static void drawEqSideScale(bool right, uint8_t brightness)
{
    const int labelX = right ? 317 : 3;
    const int tickX  = right ? 294 : 23;
    tft.setTextDatum(right ? TR_DATUM : TL_DATUM);

    for (int db = -12; db <= 12; db += 3)
    {
        int yy = EQ_CENTER_Y -
            static_cast<int>(roundf(db / 12.0f * (EQ_RANGE - 5)));
        const bool major = (db == 12 || db == 0 || db == -12);
        const uint16_t tick = scaleColor565(
            major ? tft.color565(235, 34, 38)
                  : tft.color565(147, 151, 159), brightness);
        tft.drawFastHLine(tickX, yy, 4, tick);

        if (db % 6 == 0)
        {
            char label[5];
            if (db > 0) snprintf(label, sizeof(label), "+%d", db);
            else snprintf(label, sizeof(label), "%d", db);
            tft.setTextColor(scaleColor565(
                tft.color565(205, 208, 213), brightness), TFT_BLACK);
            // Font 1 fits the narrow 18 px left/right margins.
            tft.drawString(label, labelX, yy - 4, 1);
        }
    }
}

void drawScreenHeader(uint8_t brightness)
{
    // Persistent header. Reserve the first 30 rows for it on EVERY screen.
    tft.fillRect(0, 0, 320, 30, TFT_BLACK);
    tft.drawFastHLine(3, 26, 314,
        scaleColor565(tft.color565(48, 49, 53), brightness));
    tft.drawFastHLine(3, 27, 314,
        scaleColor565(tft.color565(115, 17, 22), brightness));
    // Subtle recessed chrome enclosure shared by the visualizations.
    // Edge-only strokes keep the moving bars untouched.
    tft.drawFastVLine(0, 30, 209,
        scaleColor565(tft.color565(67, 68, 73), brightness));
    tft.drawFastVLine(319, 30, 209,
        scaleColor565(tft.color565(30, 30, 34), brightness));
    tft.drawFastHLine(1, 239, 318,
        scaleColor565(tft.color565(95, 96, 102), brightness));

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(
        scaleColor565(tft.color565(208, 208, 211), brightness),
        TFT_BLACK
    );
    const char* title =
        visualMode == EQUALIZER
          ? (uiState == EQ_EDIT_MODE ? "EQUALIZER  /  EDIT" : "EQUALIZER  /  PLAY")
          : visualModeNames[visualMode];
    tft.drawString(title, 5, 7, 2);

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(
        scaleColor565(
            audioProcessingEnabled ? tft.color565(235, 35, 39)
                                   : tft.color565(145, 145, 149),
            brightness
        ),
        TFT_BLACK
    );
    tft.drawString(audioProcessingEnabled ? "DSP ON" : "DSP OFF",
                   315, 8, 2);
}

void drawEqualizerFull(uint8_t brightness)
{
    tft.fillScreen(TFT_BLACK);
    for (uint8_t i = 0; i < EQ_BAND_COUNT; ++i)
        eqTrackCacheValid[i] = false;

    drawScreenHeader(brightness);

    tft.startWrite();
    for (uint8_t i = 0; i < EQ_BAND_COUNT; ++i)
        drawEqBand(i, brightness);
    tft.endWrite();
    // Draw the side scales last so the outer band shadows cannot erase ticks.
    drawEqSideScale(false, brightness);
    drawEqSideScale(true, brightness);

    tft.drawFastHLine(3, 238, 314,
        scaleColor565(tft.color565(87, 15, 19), brightness));
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

        tft.fillRect(0, 91, 320,
                     overlayTextLine2 != nullptr ? 50 : 30,
                     TFT_BLACK);

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
        else
        {
            resetVisualState();
        }

        return;

    }

    tft.fillRect(0, 91, 320, overlayHeight + 5, TFT_BLACK);

    float brightness = 1.0f;

    constexpr unsigned long FADE_IN_MS = 1;
    constexpr unsigned long HOLD_UNTIL_MS = OVERLAY_DURATION;

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

            96,

            2

        );

        tft.drawString(

            overlayTextLine2,

            160,

            116,

            2

        );

    }

    else

    {

        tft.drawString(

            overlayText,

            160,

            99,

            2

        );

    }

}

void showVisualModeOverlay()
{
    // Kept for compatibility. Screen titles now live in the fixed header.
    drawScreenHeader();
}

// ==================================================

// RESET VISUAL STATE

// ==================================================

// Shared gauge state must be declared before resetVisualState().
static bool gaugeFaceReady = false;
static float gaugeLevel[2] = {0.0f, 0.0f};
static float gaugeDrawnAngle[2] = {-999.0f, -999.0f};
static uint32_t gaugeLastUpdate = 0;
static bool retroFaceReady = false;
static float retroLevel[2] = {0.0f, 0.0f};
static float retroAngle[2] = {-999.0f, -999.0f};
static uint32_t retroLastUpdate = 0;


static uint8_t waClassicPrev[BAND_COUNT] = {};
static uint8_t waFirePrev[BAND_COUNT] = {};
static float waClassicPeak[BAND_COUNT] = {};
static int8_t waClassicPeakDrawn[BAND_COUNT] = {};
static uint32_t waClassicTick = 0;
static int16_t scopePrevY[2][160] = {};
static bool scopePrevValid = false;
static uint32_t scopeLastFrame = 0;

void resetVisualState()

{
    scopePrevValid = false;
    scopeLastFrame = 0;
    gaugeFaceReady = false;
    retroFaceReady = false;
    memset(waClassicPrev, 0, sizeof(waClassicPrev));
    memset(waFirePrev, 0, sizeof(waFirePrev));
    memset(waClassicPeak, 0, sizeof(waClassicPeak));
    memset(waClassicPeakDrawn, 0, sizeof(waClassicPeakDrawn));
    waClassicTick = millis();
    retroLevel[0] = retroLevel[1] = 0.0f;
    retroAngle[0] = retroAngle[1] = -999.0f;
    gaugeLevel[0] = gaugeLevel[1] = 0.0f;
    gaugeDrawnAngle[0] = gaugeDrawnAngle[1] = -999.0f;


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
// STEREO ANALOG GAUGE - STATIC 3D FACE, DYNAMIC NEEDLES
// ==================================================

// Stereo needle meters: two matching inset aluminium panels.
// Fixed faces are painted once; needle movement touches only the clear
// centre of each dial and never overdraws tick marks or labels.
static constexpr int GAUGE_PIVOT_Y = 193;
static constexpr int GAUGE_TIP_R = 83;
static constexpr float GAUGE_START_DEG = 205.0f;
static constexpr float GAUGE_SWEEP_DEG = 120.0f;  // inset at right scale endpoint
static constexpr uint16_t GAUGE_REFRESH_MS = 33; // ~30 FPS; reduce TFT load
static constexpr float GAUGE_VISUAL_GAIN_DB = 3.0f;

static int gaugeCX(uint8_t side) { return side ? 240 : 80; }
static uint16_t gaugeInnerColor() { return tft.color565(240, 241, 243); }

static void gaugeNeedle(uint8_t side, float angle, bool erase)
{
    const int cx = gaugeCX(side);
    const int cy = GAUGE_PIVOT_Y;
    const float a = angle * DEG_TO_RAD;
    const int tipX = cx + lroundf(cosf(a) * GAUGE_TIP_R);
    const int tipY = cy + lroundf(sinf(a) * GAUGE_TIP_R);
    if (erase)
    {
        // Restore each EXACT old stroke, including the displaced shadow
        // and specular highlight. This eliminates persistent stair-step trails.
        const uint16_t bg = gaugeInnerColor();
        tft.drawLine(cx + 2, cy + 2, tipX + 2, tipY + 2, bg);
        tft.drawLine(cx + 1, cy, tipX + 1, tipY, bg);
        tft.drawLine(cx - 1, cy, tipX - 1, tipY, bg);
        tft.drawLine(cx, cy, tipX, tipY, bg);
        tft.drawLine(cx - 1, cy - 1, tipX - 1, tipY - 1, bg);
        return;
    }

    // 3D red lacquer needle: dark offset shadow, black lateral edges,
    // bright red body and hairline specular highlight.
    tft.drawLine(cx + 2, cy + 2, tipX + 2, tipY + 2,
                 tft.color565(125, 130, 140));
    tft.drawLine(cx + 1, cy, tipX + 1, tipY,
                 tft.color565(49, 10, 16));
    tft.drawLine(cx - 1, cy, tipX - 1, tipY,
                 tft.color565(116, 10, 19));
    tft.drawLine(cx, cy, tipX, tipY,
                 tft.color565(233, 17, 30));
    tft.drawLine(cx - 1, cy - 1, tipX - 1, tipY - 1,
                 tft.color565(255, 105, 109));
}

static void gaugeBoss(uint8_t side)
{
    const int cx = gaugeCX(side);
    const int cy = GAUGE_PIVOT_Y;
    tft.fillCircle(cx + 2, cy + 2, 10, tft.color565(60, 62, 69));
    tft.fillCircle(cx, cy, 10, tft.color565(130, 133, 140));
    tft.fillCircle(cx - 1, cy - 1, 8, tft.color565(215, 216, 219));
    tft.fillCircle(cx - 2, cy - 3, 5, TFT_WHITE);
    tft.drawCircle(cx, cy, 10, tft.color565(201, 29, 36));
}

static void drawGaugeFace(uint8_t side)
{
    const int cx = gaugeCX(side);
    const int left = cx - 76;
    const int top = 38;
    const int width = 152;
    const int height = 192;

    // Shadow + double bevel, controlled to fit both screen edges.
    tft.fillRoundRect(left + 2, top + 3, width, height, 9,
                      tft.color565(15, 15, 18));
    tft.fillRoundRect(left, top, width, height, 9,
                      tft.color565(88, 91, 97));
    tft.fillRoundRect(left + 2, top + 2, width - 4, height - 4, 7,
                      tft.color565(213, 215, 219));
    tft.drawRoundRect(left + 1, top + 1, width - 2, height - 2, 8,
                      tft.color565(249, 249, 251));
    // A dark recessed bezel lends depth to a pale meter dial.
    tft.fillRoundRect(left + 5, top + 6, width - 10, height - 12, 7,
                      tft.color565(48, 50, 56));
    tft.fillRoundRect(left + 8, top + 9, width - 16, height - 18, 5,
                      gaugeInnerColor());

    // Static machined bevel: parallel specular lip and recessed shadow.
    tft.drawFastVLine(left + 9, top + 16, height - 34,
                      tft.color565(252, 252, 254));
    tft.drawFastVLine(left + width - 10, top + 16, height - 34,
                      tft.color565(116, 120, 129));
    tft.drawFastVLine(left + width - 11, top + 16, height - 34,
                      tft.color565(182, 186, 193));
    tft.drawFastHLine(left + 14, top + height - 15, width - 28,
                      tft.color565(130, 134, 143));

    // Bright upper chrome edge and darker underside.
    tft.drawFastHLine(left + 13, top + 10, width - 26,
                      TFT_WHITE);
    tft.drawFastHLine(left + 13, top + height - 12, width - 26,
                      tft.color565(135, 137, 143));

    // Fine professional sweep, matching the actual needle geometry.
    // All ticks remain beyond the 83 px needle reach.
    for (int i = 0; i <= 30; ++i)
    {
        const float angle = (GAUGE_START_DEG +
                              GAUGE_SWEEP_DEG * i / 30.0f) * DEG_TO_RAD;
        const bool major = (i % 5 == 0);
        const int r0 = major ? 88 : 92;
        const int r1 = 97;
        // Keep the far-left/right ticks within the silver panel.
        const int x0 = cx + lroundf(cosf(angle) * r0);
        const int x1 = cx + lroundf(cosf(angle) * r1);
        const int y0 = GAUGE_PIVOT_Y + lroundf(sinf(angle) * r0);
        const int y1 = GAUGE_PIVOT_Y + lroundf(sinf(angle) * r1);
        if (x0 <= left + 9 || x0 >= left + width - 9 ||
            x1 <= left + 9 || x1 >= left + width - 9)
            continue;
        const uint16_t c = i > 24
            ? tft.color565(195, 22, 30)
            : tft.color565(72, 75, 82);
        tft.drawLine(x0, y0, x1, y1, c);
    }
    // Scale labels are deliberately placed above the needle envelope.
    // A needle reaches only up to y=110, so text in y=60..80 is safe.
    tft.setTextDatum(MC_DATUM);
    const char* labels[5] = {"-42", "-30", "-18", "-6", "0"};
    const int labelOffset[5] = {-53, -26, 0, 26, 53};
    for (int j = 0; j < 5; ++j)
    {
        tft.setTextColor(j == 4 ? tft.color565(187, 26, 32)
                                : tft.color565(76, 79, 86),
                         gaugeInnerColor());
        tft.drawString(labels[j], cx + labelOffset[j], 78, 1);
    }
    tft.drawFastHLine(cx - 58, 89, 116,
                      tft.color565(184, 187, 194));
    tft.drawFastHLine(cx - 58, 90, 116,
                      tft.color565(255, 255, 255));

    // Channel designation under the mechanical pivot.
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(tft.color565(70, 72, 78), gaugeInnerColor());
    tft.drawString(side ? "RIGHT" : "LEFT", cx, 213, 2);
}

static void drawDualGauge()
{
    if (!gaugeFaceReady)
    {
        // Avoid one long TFT SPI transaction during a full redraw.
        drawGaugeFace(0);
        delay(1); // Let BT/FreeRTOS tasks run before the second face.
        drawGaugeFace(1);
        delay(1);
        gaugeFaceReady = true;
        gaugeLastUpdate = millis();
        gaugeDrawnAngle[0] = gaugeDrawnAngle[1] = -999.0f;
    }

    const uint32_t now = millis();
    if (now - gaugeLastUpdate < GAUGE_REFRESH_MS) return;
    const float dt = constrain(static_cast<float>(now - gaugeLastUpdate),
                               1.0f, 70.0f);
    gaugeLastUpdate = now;
    const float in[2] = {stereoPeakLeft / 32768.0f,
                         stereoPeakRight / 32768.0f};
    // Short independent draw calls; never hold the SPI transaction over
    // both complete needle updates.
    for (uint8_t ch = 0; ch < 2; ++ch)
    {
        const float tau = in[ch] > gaugeLevel[ch] ? 12.0f : 190.0f;
        const float alpha = 1.0f - expf(-dt / tau);
        gaugeLevel[ch] += (in[ch] - gaugeLevel[ch]) * alpha;

        // Display-only trim, kept modest to prevent both needles
        // saturating near the right-hand end with normal music.
        const float db = 20.0f *
            log10f(fmaxf(gaugeLevel[ch], 0.0001f)) +
            GAUGE_VISUAL_GAIN_DB;
        const float pos = (constrain(db, -42.0f, 0.0f) + 42.0f) / 42.0f;
        const float angle = constrain(GAUGE_START_DEG + pos * GAUGE_SWEEP_DEG,
                                      GAUGE_START_DEG, GAUGE_START_DEG + GAUGE_SWEEP_DEG);

        if (fabsf(angle - gaugeDrawnAngle[ch]) >= 0.8f)
        {
            if (gaugeDrawnAngle[ch] > -100.0f)
                gaugeNeedle(ch, gaugeDrawnAngle[ch], true);
            gaugeNeedle(ch, angle, false);
            gaugeBoss(ch);
            gaugeDrawnAngle[ch] = angle;
        }
    }
}

// ==================================================
// RETRO VU — illuminated ivory meter pair with machined chrome cases.
// These meters use the EXISTING stereo peak taps, no extra FFT.
// ==================================================
static constexpr int RETRO_CY = 126;
static constexpr int RETRO_PIVOT_Y = 168;
static constexpr int RETRO_NEEDLE_LENGTH = 57;
static constexpr float RETRO_FIRST_ANGLE = 208.0f;
static constexpr float RETRO_LAST_ANGLE = 332.0f;
static constexpr uint32_t RETRO_REFRESH_MS = 33;  // same stable 30fps limit
static constexpr float RETRO_SENSITIVITY_DB = 3.0f;
static int retroCenter(uint8_t ch) { return ch ? 240 : 80; }
static uint16_t retroIvory() { return tft.color565(255, 226, 161); }

static void retroNeedle(uint8_t ch, float deg, bool erase)
{
    const int x = retroCenter(ch), y = RETRO_PIVOT_Y;
    const float a = deg * DEG_TO_RAD;
    const int tx = x + lroundf(cosf(a) * RETRO_NEEDLE_LENGTH);
    const int ty = y + lroundf(sinf(a) * RETRO_NEEDLE_LENGTH);
    if (erase)
    {
        const uint16_t bg = retroIvory();
        // Erase exactly every previous stroke (including shadow and highlight).
        tft.drawLine(x+2,y+2,tx+2,ty+2,bg);
        tft.drawLine(x+1,y,tx+1,ty,bg);
        tft.drawLine(x-1,y,tx-1,ty,bg);
        tft.drawLine(x,y,tx,ty,bg);
        tft.drawLine(x-1,y-1,tx-1,ty-1,bg);
        return;
    }
    tft.drawLine(x+2,y+2,tx+2,ty+2,tft.color565(157,105,44));
    tft.drawLine(x+1,y,tx+1,ty,tft.color565(75,29,17));
    tft.drawLine(x-1,y,tx-1,ty,tft.color565(112,21,15));
    tft.drawLine(x,y,tx,ty,tft.color565(230,34,20));
    tft.drawLine(x-1,y-1,tx-1,ty-1,tft.color565(255,125,59));
}

static void retroHub(uint8_t ch)
{
    const int x=retroCenter(ch), y=RETRO_PIVOT_Y;
    tft.fillCircle(x+1,y+2,9,tft.color565(74,56,40));
    tft.fillCircle(x,y,8,tft.color565(48,47,47));
    tft.drawCircle(x,y,8,tft.color565(151,134,112));
    tft.fillCircle(x-2,y-3,3,tft.color565(143,139,128));
}

static void retroScrew(int x,int y)
{
    tft.fillCircle(x+1,y+1,4,tft.color565(44,40,39));
    tft.fillCircle(x,y,4,tft.color565(177,174,171));
    tft.drawCircle(x,y,4,tft.color565(71,69,67));
    tft.drawLine(x-2,y+2,x+2,y-2,tft.color565(51,48,48));
    tft.drawPixel(x-2,y-2,TFT_WHITE);
}

// Smooth, bold italic arcade lettering inspired by the reference logo.
// 2-bit antialiased glyphs embedded in flash; no extra TFT fonts or SPI bitmap assets.
static const uint8_t retroLabelLEFT[] PROGMEM = {
    0x0B, 0xE0, 0x00, 0x3F, 0xFF, 0xE0, 0x7F, 0xFF, 0xC7, 0xFF, 0xFF, 0xF0, 0xFD, 0x00, 0x03, 0xFF,
    0xFD, 0x0B, 0xFF, 0xFC, 0xBF, 0xFF, 0xFF, 0x0F, 0xC0, 0x00, 0x7F, 0x55, 0x40, 0xBE, 0x55, 0x45,
    0x5F, 0xE5, 0x40, 0xFC, 0x00, 0x0B, 0xE0, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0xFC, 0x00, 0x1F, 0x80,
    0x00, 0xFE, 0x55, 0x00, 0xFE, 0x55, 0x00, 0x0F, 0xC0, 0x02, 0xF8, 0x00, 0x0F, 0xFF, 0xF4, 0x1F,
    0xFF, 0xF0, 0x01, 0xFC, 0x00, 0x3F, 0x40, 0x00, 0xFF, 0xFF, 0x02, 0xFF, 0xFE, 0x00, 0x2F, 0x80,
    0x03, 0xF0, 0x00, 0x1F, 0xC0, 0x00, 0x2F, 0x40, 0x00, 0x02, 0xF4, 0x00, 0x3F, 0x00, 0x02, 0xF8,
    0x00, 0x03, 0xF0, 0x00, 0x00, 0x3F, 0x00, 0x07, 0xF5, 0x54, 0x3F, 0x95, 0x50, 0x3F, 0x00, 0x00,
    0x03, 0xF0, 0x00, 0xBF, 0xFF, 0xC3, 0xFF, 0xFF, 0x07, 0xF0, 0x00, 0x00, 0x7F, 0x00, 0x0F, 0xFF,
    0xFC, 0x3F, 0xFF, 0xE0, 0xBE, 0x00, 0x00, 0x0B, 0xE0, 0x00
};
static const uint8_t retroLabelRIGHT[] PROGMEM = {
    0x07, 0xFF, 0x90, 0x01, 0xF8, 0x00, 0x6F, 0xF9, 0x00, 0xF8, 0x01, 0xF8, 0x7F, 0xFF, 0xFE, 0x0B,
    0xFF, 0xF8, 0x02, 0xF8, 0x03, 0xFF, 0xFF, 0x41, 0xFC, 0x02, 0xF8, 0xFF, 0xFF, 0xFF, 0x0F, 0xE6,
    0xFD, 0x03, 0xF4, 0x0F, 0xF5, 0x5B, 0x02, 0xF8, 0x02, 0xF4, 0xAA, 0xFF, 0xA9, 0x0F, 0xC0, 0xFD,
    0x03, 0xF0, 0x3F, 0xC0, 0x00, 0x03, 0xF4, 0x03, 0xF4, 0x00, 0xFC, 0x00, 0x1F, 0xC0, 0xFC, 0x07,
    0xF0, 0x7F, 0x00, 0x00, 0x03, 0xF0, 0x03, 0xF0, 0x00, 0xFC, 0x00, 0x1F, 0xD6, 0xF8, 0x07, 0xE0,
    0xFE, 0x00, 0x54, 0x07, 0xFF, 0xFF, 0xF0, 0x01, 0xFC, 0x00, 0x2F, 0xFF, 0xE0, 0x0B, 0xE0, 0xFD,
    0x03, 0xFF, 0x07, 0xFF, 0xFF, 0xE0, 0x02, 0xF8, 0x00, 0x3F, 0xFF, 0xD0, 0x0F, 0xD0, 0xFC, 0x07,
    0xFE, 0x0B, 0xF6, 0x5F, 0xD0, 0x02, 0xF4, 0x00, 0x3F, 0x0B, 0xF0, 0x0F, 0xC0, 0xFD, 0x00, 0xBD,
    0x0F, 0xD0, 0x0F, 0xD0, 0x03, 0xF4, 0x00, 0x7F, 0x03, 0xF4, 0x1F, 0xC0, 0xFE, 0x00, 0xBD, 0x0F,
    0xC0, 0x0F, 0xC0, 0x03, 0xF0, 0x00, 0x7E, 0x02, 0xF8, 0x1F, 0x80, 0xBF, 0x95, 0xFC, 0x0F, 0xC0,
    0x1F, 0xC0, 0x07, 0xF0, 0x00, 0xBE, 0x01, 0xFC, 0x2F, 0x80, 0x2F, 0xFF, 0xFC, 0x1F, 0xC0, 0x2F,
    0x80, 0x0B, 0xE0, 0x00, 0xBD, 0x00, 0xFC, 0x2F, 0x40, 0x07, 0xFF, 0x90, 0x1F, 0x40, 0x2F, 0x40,
    0x0B, 0xD0, 0x00
};

static void drawRetroPlateLabel(uint8_t ch, int cx)
{
    const uint8_t* const glyph = ch ? retroLabelRIGHT : retroLabelLEFT;
    const int w = ch ? 60 : 46;
    const int h = ch ? 13 : 12;
    const int x0 = cx - w / 2;
    const int y0 = 192 + (12 - h) / 2;

    // Keep all pixels inside the steel nameplate, well below needle travel.
    // Cool lower-right outline plus warm silver letterface, like an italic
    // 1990s media-player logo but legible at 320x240.
    const uint16_t shadow = tft.color565(35, 45, 63);
    const uint16_t edge = tft.color565(79, 71, 91);
    const uint16_t inkMid = tft.color565(220, 217, 228);
    const uint16_t inkHigh = tft.color565(255, 253, 239);

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const int idx = y * w + x;
            const uint8_t packed = pgm_read_byte(glyph + idx / 4);
            const uint8_t a = (packed >> (6 - 2 * (idx % 4))) & 3;
            if (a >= 2 && y0+y+1 <= 203)
                tft.drawPixel(x0+x+1, y0+y+1, shadow);
        }

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const int idx = y * w + x;
            const uint8_t packed = pgm_read_byte(glyph + idx / 4);
            const uint8_t a = (packed >> (6 - 2 * (idx % 4))) & 3;
            if (a)
                tft.drawPixel(x0+x, y0+y,
                    a == 1 ? edge : (a == 2 ? inkMid : inkHigh));
        }
}

static void drawRetroFace(uint8_t ch)
{
    const int cx=retroCenter(ch);
    // 150-pixel body, fitting two perfectly symmetric faces in 320 pixels.
    tft.fillCircle(cx+2,RETRO_CY+3,77,tft.color565(20,18,19));
    tft.fillCircle(cx,RETRO_CY,76,tft.color565(66,64,63));
    tft.fillCircle(cx-1,RETRO_CY-1,74,tft.color565(181,177,171));
    tft.drawCircle(cx,RETRO_CY,75,tft.color565(246,244,238));
    tft.drawCircle(cx,RETRO_CY,72,tft.color565(103,99,94));
    tft.fillCircle(cx,RETRO_CY,69,tft.color565(33,27,25));
    tft.fillCircle(cx,RETRO_CY,65,tft.color565(231,166,58));
    tft.fillCircle(cx,RETRO_CY,61,tft.color565(254,210,116));
    tft.fillCircle(cx,RETRO_CY,58,retroIvory());
    // Concentrated overhead lamp and recessed concentric brass bands.
    tft.drawArc(cx,RETRO_CY,66,64,210,330,tft.color565(255,220,138),TFT_BLACK);
    tft.drawArc(cx,RETRO_CY,68,67,215,325,tft.color565(91,54,27),TFT_BLACK);
    tft.fillCircle(cx,RETRO_CY-51,5,tft.color565(255,242,186));
    tft.fillCircle(cx,RETRO_CY-51,2,TFT_WHITE);
    // Scale printed on fixed, illuminated surface. Needle ends before ticks.
    for (int i=0;i<=20;i++)
    {
        const float a=(RETRO_FIRST_ANGLE+(RETRO_LAST_ANGLE-RETRO_FIRST_ANGLE)*i/20)*DEG_TO_RAD;
        const int r1=(i%5==0)?61:65, r2=69;
        const int x1=cx+lroundf(cosf(a)*r1);
        const int y1=RETRO_PIVOT_Y+lroundf(sinf(a)*r1);
        const int x2=cx+lroundf(cosf(a)*r2);
        const int y2=RETRO_PIVOT_Y+lroundf(sinf(a)*r2);
        // Clip to the inset circular dial area.
        if ((x1-cx)*(x1-cx)+(y1-RETRO_CY)*(y1-RETRO_CY)>57*57 ||
            (x2-cx)*(x2-cx)+(y2-RETRO_CY)*(y2-RETRO_CY)>62*62) continue;
        tft.drawLine(x1,y1,x2,y2,
            i>=15?tft.color565(191,23,19):tft.color565(36,30,27));
    }
    // Printed numerals ABOVE the needle sweep, deliberately not under needle.
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(tft.color565(36,30,26),retroIvory());
    tft.drawString("-20",cx-43,RETRO_CY-22,1);
    tft.drawString("-10",cx-23,RETRO_CY-38,1);
    tft.drawString("-5",cx,RETRO_CY-43,1);
    tft.setTextColor(tft.color565(176,19,16),retroIvory());
    tft.drawString("0",cx+26,RETRO_CY-38,1);
    tft.drawString("+3",cx+46,RETRO_CY-22,1);
    // A little machined plate at the bottom, outside needle's travel.
    tft.fillRoundRect(cx-45,189,90,19,3,tft.color565(61,58,56));
    tft.fillRoundRect(cx-43,190,86,15,2,tft.color565(175,171,167));
    tft.drawFastHLine(cx-40,191,80,TFT_WHITE);
    drawRetroPlateLabel(ch, cx); // LEFT / RIGHT, bold italic retro logo font
    // Decorative bezel bolts placed on machined flange.
    retroScrew(cx-54,RETRO_CY-47);
    retroScrew(cx+54,RETRO_CY-47);
    retroScrew(cx-55,RETRO_CY+46);
    retroScrew(cx+55,RETRO_CY+46);
}

static void drawRetroVU()
{
    if (!retroFaceReady)
    {
        drawRetroFace(0);
        delay(1); // preserve the SPI-safe behavior of DUAL GAUGE
        drawRetroFace(1);
        delay(1);
        retroFaceReady = true;
        retroLastUpdate = millis();
        retroAngle[0] = retroAngle[1] = -999.0f;
    }
    const uint32_t now=millis();
    if(now-retroLastUpdate < RETRO_REFRESH_MS) return;
    const float dt=constrain(float(now-retroLastUpdate),1.0f,70.0f);
    retroLastUpdate=now;
    const float levels[2]={stereoPeakLeft/32768.0f,stereoPeakRight/32768.0f};
    for(uint8_t ch=0;ch<2;ch++)
    {
        const float tau=levels[ch]>retroLevel[ch]?15.0f:210.0f;
        retroLevel[ch]+=(levels[ch]-retroLevel[ch])*(1.0f-expf(-dt/tau));
        const float db=20.0f*log10f(fmaxf(retroLevel[ch],0.0001f))+RETRO_SENSITIVITY_DB;
        const float p=(constrain(db,-42.0f,0.0f)+42.0f)/42.0f;
        const float deg=constrain(RETRO_FIRST_ANGLE+p*(RETRO_LAST_ANGLE-RETRO_FIRST_ANGLE),
                                  RETRO_FIRST_ANGLE,RETRO_LAST_ANGLE);
        if(fabsf(deg-retroAngle[ch])>=0.85f)
        {
            if(retroAngle[ch]>-100.0f) retroNeedle(ch,retroAngle[ch],true);
            retroNeedle(ch,deg,false);
            retroHub(ch);
            retroAngle[ch]=deg;
        }
    }
}

// ==================================================
// WINAMP-INSPIRED VISUALIZATIONS
// 16 bands; existing FFT only. No additional PCM processing.
// Incremental updates: unchanged LED segments are never redrawn.
// ==================================================
static constexpr uint8_t WA_BANDS = BAND_COUNT;
static constexpr int WA_FIRST_Y = 38;
static constexpr int WA_BOTTOM = 227;
static constexpr int WA_SEG_H = 5;
static constexpr int WA_SEG_PITCH = 7;
static constexpr int WA_SEG_COUNT = 26;

static uint16_t winampLedColor(bool fire, int seg)
{
    if (fire)
    {
        if (seg >= 22) return tft.color565(247, 58, 27);
        if (seg >= 15) return tft.color565(246, 135, 25);
        return tft.color565(237, 177, 49);
    }
    if (seg >= 23) return tft.color565(245, 53, 44);
    if (seg >= 19) return tft.color565(244, 184, 38);
    return tft.color565(85, 224, 76);
}

static void drawWinampBars(bool fire)
{
    uint8_t* previous = fire ? waFirePrev : waClassicPrev;
    const uint32_t now = millis();
    const float delta = constrain(float(now - waClassicTick), 0.0f, 85.0f);
    if (!fire) waClassicTick = now;

    tft.startWrite();
    for (int i = 0; i < WA_BANDS; ++i)
    {
        const int x0 = i * 320 / WA_BANDS;
        const int x1 = (i + 1) * 320 / WA_BANDS;
        const int x = x0 + 2;
        const int width = x1 - x0 - 4;
        const float n = constrain(smoothedBands[i] / fmaxf(autoPeak, 1.0f), 0.0f, 1.0f);
        const int active = constrain(int(powf(n, fire ? 1.43f : 1.60f) * WA_SEG_COUNT + .5f), 0, WA_SEG_COUNT);
        const int old = previous[i];
        const int from = min(old, active), to = max(old, active);
        for (int seg = from; seg < to; ++seg)
        {
            const int y = WA_BOTTOM - (seg + 1) * WA_SEG_PITCH;
            if (seg < active)
            {
                const uint16_t c = winampLedColor(fire, seg);
                tft.fillRect(x, y, width, WA_SEG_H, c);
                // One-pixel highlight + shade gives LCD-feasible 3D depth.
                tft.drawFastHLine(x + 1, y, width - 2,
                    fire ? tft.color565(255, 207, 108) : tft.color565(168, 249, 151));
                tft.drawFastVLine(x + width - 1, y + 1, WA_SEG_H - 1,
                    fire ? tft.color565(105, 43, 19) : tft.color565(21, 78, 28));
            }
            else tft.fillRect(x, y, width, WA_SEG_H, TFT_BLACK);
        }
        previous[i] = active;

        if (!fire)
        {
            const int oldPeak = waClassicPeakDrawn[i];
            float peak = waClassicPeak[i] - delta * 0.027f;
            if (peak < 0.0f) peak = 0.0f;
            if (active > peak) peak = float(active);
            waClassicPeak[i] = peak;
            const int mark = min(WA_SEG_COUNT, int(ceilf(peak)));
            if (oldPeak != mark)
            {
                // Erase just old peak; recover the segment underneath.
                if (oldPeak > 0)
                {
                    const int oy = WA_BOTTOM - oldPeak * WA_SEG_PITCH - 2;
                    tft.fillRect(x, oy, width, 2, TFT_BLACK);
                }
                if (mark > 0)
                {
                    const int py = WA_BOTTOM - mark * WA_SEG_PITCH - 2;
                    tft.fillRect(x, py, width, 2, tft.color565(236, 236, 224));
                }
                waClassicPeakDrawn[i] = mark;
            }
        }
    }
    tft.endWrite();
}

// ==================================================
// WINAMP-STYLE REAL PCM OSCILLOSCOPES
// Draw 160 x 2px wide points; update <= 30 FPS.
// Restore only old trace pixels, then grid, then new trace.
// ==================================================
static constexpr int SCOPE_POINTS = 160;
static constexpr int SCOPE_DISPLAY_GAIN = 5; // retain current scope sensitivity
static constexpr int SCOPE_Y_TOP = 38;
static constexpr int SCOPE_Y_BOTTOM = 233;

static void drawScope(bool stereo)
{
    const uint32_t now = millis();
    if ((uint32_t)(now - scopeLastFrame) < 33) return;
    scopeLastFrame = now;

    // Snapshot the circular buffer read pointer once. No allocation/locks.
    const uint16_t end = scopeWriteIndex;
    int16_t l[SCOPE_POINTS], r[SCOPE_POINTS];
    // Pick a rising zero crossing in the recent 320 samples for a less
    // wandering waveform. The final visible window contains 160 samples.
    uint16_t start = (end - 200) & (SCOPE_RING_SIZE - 1);
    int16_t previous = scopeRingL[start];
    for (uint16_t i = 1; i < 40; ++i)
    {
        uint16_t j = (start + i) & (SCOPE_RING_SIZE - 1);
        int16_t current = scopeRingL[j];
        if (previous < 0 && current >= 0) { start = j; break; }
        previous = current;
    }
    for (int i = 0; i < SCOPE_POINTS; ++i)
    {
        const uint16_t j = (start + i) & (SCOPE_RING_SIZE - 1);
        l[i] = scopeRingL[j];
        r[i] = scopeRingR[j];
    }

    // Separate amplitude control for display, never touches output PCM.
    const int cy[2] = {stereo ? 90 : 135, stereo ? 185 : 135};
    const int half = stereo ? 39 : 76;
    int16_t currentY[2][SCOPE_POINTS];
    for (int i = 0; i < SCOPE_POINTS; ++i)
    {
        const int16_t m = (int16_t)((int32_t(l[i]) + int32_t(r[i])) / 2);
        const int32_t value0 = stereo ? l[i] : m;
        const int32_t value1 = stereo ? r[i] : m;
        currentY[0][i] = constrain(cy[0] - (value0 * half * SCOPE_DISPLAY_GAIN) / 32768,
                                   stereo ? 44 : 48, stereo ? 133 : 221);
        currentY[1][i] = constrain(cy[1] - (value1 * half * SCOPE_DISPLAY_GAIN) / 32768, 140, 226);
    }

    tft.startWrite();
    // Old traces are erased with short lines. Reapply background grid
    // afterward, instead of clearing 320x200 pixels every frame.
    if (scopePrevValid)
    {
        for (int ch = 0; ch < (stereo ? 2 : 1); ++ch)
            for (int i = 1; i < SCOPE_POINTS; ++i)
            {
                tft.drawLine((i-1)*2, scopePrevY[ch][i-1], i*2, scopePrevY[ch][i], TFT_BLACK);
                if (!stereo)
                    tft.drawLine((i-1)*2, scopePrevY[ch][i-1]+1, i*2, scopePrevY[ch][i]+1, TFT_BLACK);
            }
    }
    else tft.fillRect(0, SCOPE_Y_TOP, 320, SCOPE_Y_BOTTOM-SCOPE_Y_TOP, TFT_BLACK);

    const uint16_t grid = stereo ? tft.color565(20,45,53) : tft.color565(22,43,22);
    const uint16_t axis = stereo ? tft.color565(43,77,84) : tft.color565(39,75,36);
    for (int x = 0; x <= 319; x += 40)
        tft.drawFastVLine(x, SCOPE_Y_TOP, SCOPE_Y_BOTTOM-SCOPE_Y_TOP, grid);
    for (int y = 45; y < SCOPE_Y_BOTTOM; y += 20)
        tft.drawFastHLine(0, y, 320, grid);
    tft.drawFastHLine(0, cy[0], 320, axis);
    if (stereo) tft.drawFastHLine(0, cy[1], 320, axis);

    for (int i = 1; i < SCOPE_POINTS; ++i)
    {
        const int x0 = (i - 1) * 2, x1 = i * 2;
        if (!stereo)
        {
            tft.drawLine(x0, currentY[0][i-1]+1, x1, currentY[0][i]+1, tft.color565(24,90,25));
            tft.drawLine(x0, currentY[0][i-1], x1, currentY[0][i], tft.color565(105,250,95));
        }
        else
        {
            tft.drawLine(x0, currentY[0][i-1], x1, currentY[0][i], tft.color565(62,237,225));
            tft.drawLine(x0, currentY[1][i-1], x1, currentY[1][i], tft.color565(255,170,69));
        }
    }
    if (stereo)
    {
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(tft.color565(75,233,222), TFT_BLACK);
        tft.drawString("L", 6, 47, 2);
        tft.setTextColor(tft.color565(255,167,76), TFT_BLACK);
        tft.drawString("R", 6, 143, 2);
    }
    tft.endWrite();
    memcpy(scopePrevY, currentY, sizeof(currentY));
    scopePrevValid = true;
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

        case DUAL_GAUGE:
            drawDualGauge();
            break;

        case RETRO_VU:
            drawRetroVU();
            break;

        case WINAMP_CLASSIC:
            drawWinampBars(false);
            break;

        case WINAMP_FIRE:
            drawWinampBars(true);
            break;

        case SCOPE_CRT:
            drawScope(false);
            break;

        case SCOPE_STEREO:
            drawScope(true);
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

