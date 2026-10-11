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

// EC11 adaptive detent decoder, derived from the user's serial traces.
// Typical stopped states were AB=10 with 3 net quarter-steps and AB=11
// with 0. Mechanical detents thus do not always end on a full 4-edge cycle.
// We recognize full cycles immediately AND commit a 2-3 edge remainder after
// the shaft has been electrically quiet. Alternating contact bounce cancels.
static constexpr uint32_t ENCODER_SETTLE_US = 45000UL;
static constexpr int8_t ENCODER_EDGES_PER_CYCLE = 4;
static constexpr int8_t ENCODER_MIN_REMAINDER = 2;

volatile int8_t encoderAccumulator = 0;   // -3..+3 remaining quarter-steps
volatile int16_t encoderPendingSteps = 0; // ISR -> loop step queue
volatile uint8_t encoderState = 3;
volatile uint32_t encoderLastEdgeUs = 0;
volatile uint32_t encoderTransitionCount = 0;
volatile uint32_t encoderInvalidCount = 0;

// Set to 1 to print one short report per mechanical detent. Zero by default,
// so Serial output cannot slow down normal input responsiveness.
#define EC11_DEBUG 0

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
    static const int8_t transitions[16] = {
         0, -1, +1,  0,
        +1,  0,  0, -1,
        -1,  0,  0, +1,
         0, +1, -1,  0
    };

    const uint8_t next =
        (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
    if (next == encoderState) return;

    const int8_t delta = transitions[(encoderState << 2) | next];
    encoderState = next;
    encoderLastEdgeUs = micros();
    ++encoderTransitionCount;

    if (delta == 0) {
        // Both pins changed since last observation. Direction is unknown.
        // Drop the incomplete part rather than fabricating a step.
        ++encoderInvalidCount;
        encoderAccumulator = 0;
        return;
    }

    encoderAccumulator += delta;
    if (encoderAccumulator >= ENCODER_EDGES_PER_CYCLE) {
        encoderAccumulator -= ENCODER_EDGES_PER_CYCLE;
        if (encoderPendingSteps < 100) ++encoderPendingSteps;
    } else if (encoderAccumulator <= -ENCODER_EDGES_PER_CYCLE) {
        encoderAccumulator += ENCODER_EDGES_PER_CYCLE;
        if (encoderPendingSteps > -100) --encoderPendingSteps;
    }
}

void updateEncoder()
{
    // Finalize a 2- or 3-edge mechanical detent once contacts settle.
    // No prints or busy waits in the ISR. Counters copied atomically.
    const uint32_t nowUs = micros();
    int8_t step = 0;
#if EC11_DEBUG
    int8_t settledRemainder = 0;
    uint8_t settledAB = 0;
    uint32_t transitionsSeen = 0;
    uint32_t invalidSeen = 0;
    bool committedRemainder = false;
#endif
    noInterrupts();
    if (encoderAccumulator != 0 &&
        static_cast<uint32_t>(nowUs - encoderLastEdgeUs) >= ENCODER_SETTLE_US) {
#if EC11_DEBUG
        settledRemainder = encoderAccumulator;
        settledAB = encoderState;
        transitionsSeen = encoderTransitionCount;
        invalidSeen = encoderInvalidCount;
        committedRemainder = true;
#endif
        if (encoderAccumulator >= ENCODER_MIN_REMAINDER &&
            encoderPendingSteps < 100) {
            ++encoderPendingSteps;
        } else if (encoderAccumulator <= -ENCODER_MIN_REMAINDER &&
                   encoderPendingSteps > -100) {
            --encoderPendingSteps;
        }
        // A single leftover edge is electrical wobble, not a detent.
        encoderAccumulator = 0;
    }
    if (encoderPendingSteps > 0) {
        --encoderPendingSteps;
        step = +1;
    } else if (encoderPendingSteps < 0) {
        ++encoderPendingSteps;
        step = -1;
    }
    interrupts();

#if EC11_DEBUG
    if (committedRemainder) {
        Serial.printf("[EC11] rest AB=%u%u remainder=%d transitions=%lu invalid=%lu\n",
                      (settledAB >> 1) & 1, settledAB & 1,
                      static_cast<int>(settledRemainder),
                      static_cast<unsigned long>(transitionsSeen),
                      static_cast<unsigned long>(invalidSeen));
    }
#endif
    if (step != 0) onRotate(step);
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
