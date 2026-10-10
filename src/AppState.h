#pragma once
#include <Arduino.h>
#include "VisualMode.h"

enum UiState { PLAYBACK_MODE, EQ_EDIT_MODE };
enum SignalScreenState { SCREEN_WAITING_BT, SCREEN_WAITING_AUDIO, SCREEN_RUNNING };

extern VisualMode visualMode;
extern UiState uiState;
extern uint8_t selectedEqBand;
extern SignalScreenState signalScreenState;
extern bool waitingForAudio;
extern volatile bool btConnected;
extern volatile bool btConnectionChanged;
extern volatile uint32_t lastAudioSignalMs;
extern volatile bool audioProcessingEnabled;

constexpr float AUDIO_SILENCE_THRESHOLD = 0.0020f;
constexpr unsigned long AUDIO_SIGNAL_TIMEOUT_MS = 2000;
extern const char* const TEXT_WAITING_BT;
extern const char* const TEXT_WAITING_AUDIO;
