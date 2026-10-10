#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "VisualMode.h"
#include "SpectrumAnalyzer.h"
#include "Equalizer.h"
#include "AppState.h"

extern TFT_eSPI tft;
extern VisualMode visualMode;
extern UiState uiState;
extern uint8_t selectedEqBand;
extern SignalScreenState signalScreenState;

void drawEqBand(uint8_t i, uint8_t brightness = 255);
void drawEqualizerFull(uint8_t brightness = 255);
void fadeEqualizerIn();
void fadeEqualizerOut();
void preparePixelMatrix();
void resetVisualState();
void drawSpectrum();
void drawScreenHeader(uint8_t brightness = 255);
void showOverlay(const char* text);
void showOverlayTwoLines(const char* line1, const char* line2);
void drawOverlay();
void showVisualModeOverlay();
void fadeMessageIn(const char* message);
void fadeMessageOut(const char* message);
void transitionMessage(const char* from, const char* to);
