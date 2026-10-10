#include "AppState.h"

VisualMode visualMode = CLASSIC_LED;
UiState uiState = PLAYBACK_MODE;
uint8_t selectedEqBand = 0;
SignalScreenState signalScreenState = SCREEN_WAITING_BT;
bool waitingForAudio = true;
volatile bool btConnected = false;
volatile bool btConnectionChanged = false;
volatile uint32_t lastAudioSignalMs = 0;
volatile bool audioProcessingEnabled = true;
const char* const TEXT_WAITING_BT = "Waiting for BT connection";
const char* const TEXT_WAITING_AUDIO = "Waiting for audio signal";
