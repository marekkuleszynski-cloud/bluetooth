#include <Arduino.h>
#include "AppController.h"
#include "AppState.h"
#include "InputManager.h"
#include "AudioEngine.h"
#include "BluetoothManager.h"
#include "DisplayManager.h"
#include "Equalizer.h"
#include "SpectrumAnalyzer.h"

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

        drawEqualizerFull();

    }

    drawScreenHeader();

}
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

        drawScreenHeader();

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

        drawScreenHeader();

        return;

    }

    drawEqBand(

        oldBand

    );

    drawEqBand(

        selectedEqBand

    );

}
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
        if (signalScreenState == SCREEN_RUNNING)
            drawScreenHeader();

        Serial.println(
            "Audio Processing ON"
        );
    }
    else
    {
        if (signalScreenState == SCREEN_RUNNING)
            drawScreenHeader();

        Serial.println(
            "Audio Processing OFF - true PCM bypass"
        );
    }

}
void onRotate(int step) { changeVisualMode(step); }
void onEncoderPress() { handleButtonPress(); }
void onKey0Press() { setAudioProcessing(!audioProcessingEnabled); }

void enterWaitingBluetoothScreen()
{
    if (signalScreenState == SCREEN_RUNNING)
    {
        tft.fillScreen(TFT_BLACK);
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
        tft.fillScreen(TFT_BLACK);
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
        drawEqualizerFull();
    }

    drawScreenHeader();
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