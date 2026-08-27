#include "rotary_encoder.h"
#include "dfplayer.h"
#include "../mcc_generated_files/system/system.h"
#include <stddef.h>

// Raw quadrature counts per physical detent on a typical EC11 (one full
// A/B cycle - 00->01->11->10->00 - is four single-bit edges).
#define ENCODER_COUNTS_PER_DETENT 4

// How long to let the power button's contacts settle before trusting a
// falling edge as a real press.
#define POWER_DEBOUNCE_MS 30

// Quadrature transition table, indexed by (prevAB << 2) | newAB, where each
// AB is (VOL_A << 1) | VOL_B. Yields +1/-1 for a valid single-step
// transition, 0 for a repeated state or an electrically-impossible
// two-bit jump (bounce/noise). Verified by hand: summing this table over
// one full 00->01->11->10->00 cycle nets -4 (and +4 for the reverse
// cycle), so ENCODER_COUNTS_PER_DETENT above matches it.
static const int8_t QUAD_TABLE[16] = {
    0, -1,  1,  0,
    1,  0,  0, -1,
   -1,  0,  0,  1,
    0,  1, -1,  0
};

static volatile int16_t encoderRawDelta = 0;
static uint8_t encoderLastAB = 0;

static volatile bool powerFallingEdgeSeen = false;
static bool powerPressLatched = false; // true from a recognized press until release, so we fire once per press
static RotaryEncoder_PowerButtonCallback powerButtonCallback = NULL;

// ---------------------------------------------------------------------
// ISR-side handlers - registered with the pin manager, run inside the IOC
// interrupt. Kept to just updating volatile state; no UART/DFPlayer calls
// and no blocking here.
// ---------------------------------------------------------------------

static void encoder_isr(void)
{
    uint8_t ab = (uint8_t)(((VOL_A_GetValue() ? 1 : 0) << 1) | (VOL_B_GetValue() ? 1 : 0));
    uint8_t index = (uint8_t)((encoderLastAB << 2) | ab);
    encoderRawDelta = (int16_t)(encoderRawDelta + QUAD_TABLE[index]);
    encoderLastAB = ab;
}

static void power_isr(void)
{
    powerFallingEdgeSeen = true;
}

// ---------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------

void RotaryEncoder_Init(RotaryEncoder_PowerButtonCallback onPowerButtonPressed)
{
    powerButtonCallback = onPowerButtonPressed;

    // Wired active-low; make sure they idle high whether or not the EC11
    // board itself already has pull-ups.
    VOL_A_SetPullup();
    VOL_B_SetPullup();
    POWER_SetPullup();

    encoderLastAB = (uint8_t)(((VOL_A_GetValue() ? 1 : 0) << 1) | (VOL_B_GetValue() ? 1 : 0));

    VOL_A_SetInterruptHandler(encoder_isr);
    VOL_B_SetInterruptHandler(encoder_isr);
    POWER_SetInterruptHandler(power_isr);
}

void RotaryEncoder_Tasks(void)
{
    // --- apply any full detents the ISR has accumulated ---
    PIE0bits.IOCIE = 0;
    int16_t delta = encoderRawDelta;
    int8_t steps = (int8_t)(delta / ENCODER_COUNTS_PER_DETENT);
    encoderRawDelta = (int16_t)(delta - (int16_t)(steps * ENCODER_COUNTS_PER_DETENT)); // keep any partial-detent remainder
    PIE0bits.IOCIE = 1;

    if (steps != 0)
    {
        DFPlayer_AdjustVolume(steps);
    }

    // --- debounce and dispatch the power button ---
    if (powerFallingEdgeSeen)
    {
        PIE0bits.IOCIE = 0;
        powerFallingEdgeSeen = false;
        PIE0bits.IOCIE = 1;

        if (!powerPressLatched)
        {
            __delay_ms(POWER_DEBOUNCE_MS);
            if (!POWER_GetValue()) // still low after the contacts settle = a real press
            {
                powerPressLatched = true;
                if (powerButtonCallback != NULL)
                {
                    powerButtonCallback();
                }
            }
        }
    }

    if (powerPressLatched && POWER_GetValue())
    {
        powerPressLatched = false; // released - next press can be recognized
    }
}
