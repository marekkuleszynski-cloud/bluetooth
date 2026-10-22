#include <Arduino.h>

#include "AudioEngine.h"

#include "BluetoothManager.h"

#include "DisplayManager.h"
#include "SplashScreen.h"

#include "Equalizer.h"
#include "VisualMode.h"
#include "SpectrumAnalyzer.h"
#include "InputManager.h"
#include "AppState.h"
#include "AppController.h"

// ==================================================

// LOOP

// ==================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);
    beginInput();
    beginAudioEngine();
    tft.init();
    tft.setRotation(3);
    SplashScreen::show(tft);
    fadeMessageIn(TEXT_WAITING_BT);
    signalScreenState = SCREEN_WAITING_BT;
    updateEQCoefficients();
    beginBluetooth();
}

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