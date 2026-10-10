#pragma once
#include <Arduino.h>
void beginInput();
void updateEncoder();
void updateButton();
void updateKey0();
// Consumed by AppController, never by the interrupt itself.
void onRotate(int step);
void onEncoderPress();
void onKey0Press();
