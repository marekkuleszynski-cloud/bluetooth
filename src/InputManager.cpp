#include "InputManager.h"
#include "HardwareConfig.h"

// KEY0 debounce
bool lastKey0State = HIGH;
bool stableKey0State = HIGH;
unsigned long lastKey0Change = 0;
constexpr unsigned long KEY0_DEBOUNCE_MS = 35;

// ==================================================

// ENCODER

// ==================================================

// Quadrature transitions are accumulated continuously. Waiting until A/B
// returns to HIGH/HIGH can miss a click on EC11 encoders whose mechanical
// rest position is not always the same.
volatile int8_t encoderAccumulator = 0;
volatile int16_t encoderPendingSteps = 0;
volatile uint8_t encoderState = 3;

// ==================================================

// BUTTON

// ==================================================

bool lastButtonState = HIGH;

bool stableButtonState = HIGH;

unsigned long lastButtonChange = 0;

constexpr unsigned long BUTTON_DEBOUNCE_MS =

    35;


void IRAM_ATTR encoderISR()
{
    static const int8_t table[16] =
    {
         0, -1, +1,  0,
        +1,  0,  0, -1,
        -1,  0,  0, +1,
         0, +1, -1,  0
    };

    const uint8_t newState =
        (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
    if (newState == encoderState)
        return;

    const uint8_t transition = (encoderState << 2) | newState;
    const int8_t delta = table[transition];
    encoderState = newState;

    // Invalid two-bit transitions mean that an intermediate edge was missed.
    // Discard that partial step instead of producing a false direction.
    if (delta == 0)
    {
        encoderAccumulator = 0;
        return;
    }

    encoderAccumulator += delta;

    // One full quadrature cycle is four valid edges. Register the step
    // immediately, without waiting for a specific electrical rest state.
    if (encoderAccumulator >= 4)
    {
        encoderAccumulator -= 4;
        if (encoderPendingSteps < 100)
            ++encoderPendingSteps;
    }
    else if (encoderAccumulator <= -4)
    {
        encoderAccumulator += 4;
        if (encoderPendingSteps > -100)
            --encoderPendingSteps;
    }
}

void updateEncoder()
{
    int8_t step = 0;

    noInterrupts();
    if (encoderPendingSteps > 0)
    {
        --encoderPendingSteps;
        step = +1;
    }
    else if (encoderPendingSteps < 0)
    {
        ++encoderPendingSteps;
        step = -1;
    }
    interrupts();

    if (step != 0)
        onRotate(step);
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

                onEncoderPress();

            }

        }

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

                onKey0Press();

            }

        }

    }

}
void beginInput()
{
    pinMode(ENC_A,INPUT_PULLUP);
    pinMode(ENC_B,INPUT_PULLUP);
    pinMode(ENC_BTN,INPUT_PULLUP);
    pinMode(KEY0_PIN,INPUT_PULLUP);
    lastKey0State = digitalRead(KEY0_PIN);
    stableKey0State = lastKey0State;
    encoderState = (digitalRead(ENC_A)<<1) | digitalRead(ENC_B);
    attachInterrupt(digitalPinToInterrupt(ENC_A),encoderISR,CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B),encoderISR,CHANGE);
}
