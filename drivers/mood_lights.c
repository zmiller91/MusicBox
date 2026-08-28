#include "mood_lights.h"
#include "../mcc_generated_files/system/system.h"
#include <stdbool.h>

// ---------------------------------------------------------------------
// PWM mood LEDs (LED_PWM_1/LED_PWM_2, PWM3/PWM4 on RD4/RD5)
// ---------------------------------------------------------------------

// Timer2 (MCC's PWM3/PWM4 modules use it as their period source) is now
// started by TMR2_Initialize(), called from SYSTEM_Initialize() - T2PR is
// set to 255 (PR2/T2PR alias the same register), which is what
// MOOD_LIGHTS_BRIGHTNESS_MAX (see mood_lights.h) assumes: a 10-bit duty
// range matching PWM3/PWM4_LoadDutyValue(). No timer setup needed here.

#define MOOD_LIGHTS_FADE_STEPS   60
#define MOOD_LIGHTS_FADE_STEP_MS 50 // 60*50ms = 3s end to end

void MoodLights_Init(void)
{
    // MCC left RD4/RD5 in analog mode despite routing them to PWM3/PWM4 via
    // PPS - clear it defensively so the digital output driver actually works.
    LED_PWM_1_SetDigitalMode();
    LED_PWM_2_SetDigitalMode();
    MoodLights_SetBrightness1(0);
    MoodLights_SetBrightness2(0);

    LED_3_SetDigitalOutput();
    LED_3_SetLow();
}

void MoodLights_SetBrightness1(uint16_t brightness)
{
    PWM3_LoadDutyValue(brightness);
}

void MoodLights_SetBrightness2(uint16_t brightness)
{
    PWM4_LoadDutyValue(brightness);
}

uint16_t MoodLights_GammaBrightness(uint8_t numerator, uint8_t denominator)
{
    // Cubic curve - a rough approximation of typical LED/eye gamma
    // (~2.8). numerator/denominator are always small step counts here
    // (tens, not hundreds), so numerator^3 and denominator^3 stay well
    // within uint32_t range.
    uint32_t n3 = (uint32_t)numerator * numerator * numerator;
    uint32_t d3 = (uint32_t)denominator * denominator * denominator;
    return (uint16_t)((uint32_t)MOOD_LIGHTS_BRIGHTNESS_MAX * n3 / d3);
}

void MoodLights_FadeIn(void)
{
    for (uint8_t step = 1; step <= MOOD_LIGHTS_FADE_STEPS; step++)
    {
        uint16_t brightness = MoodLights_GammaBrightness(step, MOOD_LIGHTS_FADE_STEPS);
        MoodLights_SetBrightness1(brightness);
        MoodLights_SetBrightness2(brightness);
        __delay_ms(MOOD_LIGHTS_FADE_STEP_MS);
    }
}

void MoodLights_FadeOut(void)
{
    for (uint8_t step = MOOD_LIGHTS_FADE_STEPS; step > 0; step--)
    {
        uint16_t brightness = MoodLights_GammaBrightness((uint8_t)(step - 1), MOOD_LIGHTS_FADE_STEPS);
        MoodLights_SetBrightness1(brightness);
        MoodLights_SetBrightness2(brightness);
        __delay_ms(MOOD_LIGHTS_FADE_STEP_MS);
    }
}

// ---------------------------------------------------------------------
// LED_3 firefly twinkle
// ---------------------------------------------------------------------

// Documents the cadence MoodLights_Tasks() is expected to be polled at
// (matches main.c's loop delay). The dark-gap tick counts below are sized
// against this, but nothing here actually measures real time for them -
// it's ticks, not ms. The flash itself (FIREFLY_FLASH_MS) is a real,
// blocking delay instead, since it needs to be a fixed, consistent
// duration rather than "however many main-loop iterations happen to pass".
#define MOOD_LIGHTS_TICK_MS 150

// Real fireflies' flashes are brief and fairly uniform in length - only
// the gap between them varies - so this is a single fixed duration, not a
// range. Tune to taste.
#define FIREFLY_FLASH_MS 200

#define FIREFLY_OFF_MIN_TICKS 3  // ~450ms
#define FIREFLY_OFF_MAX_TICKS 15 // ~2.25s

static uint16_t rngState;
static bool rngSeeded = false;
static bool twinkleEnabled = false;
static uint8_t fireflyTicksRemaining = FIREFLY_OFF_MIN_TICKS;

void MoodLights_SetTwinkleEnabled(bool enabled)
{
    twinkleEnabled = enabled;
    if (!enabled)
    {
        // Don't leave it stuck mid-glow when playback stops.
        LED_3_SetLow();
    }
}

// Simple 16-bit LCG - not cryptographic, just enough to make the twinkle
// timing look irregular rather than metronomic.
static uint16_t next_random(void)
{
    rngState = (uint16_t)(rngState * 25173u + 13849u);
    return rngState;
}

// Returns a value in [minVal, maxVal] inclusive.
static uint8_t random_range(uint8_t minVal, uint8_t maxVal)
{
    uint8_t span = (uint8_t)(maxVal - minVal + 1);
    return (uint8_t)(minVal + (next_random() % span));
}

void MoodLights_Tasks(void)
{
    if (!twinkleEnabled)
    {
        return;
    }

    if (!rngSeeded)
    {
        // Seeded here (first call - after PN532_Init() and friends have
        // already run) rather than in MoodLights_Init(), so the seed
        // depends on how long those real-world UART waits actually took
        // instead of being purely deterministic since power-on. TMR2
        // free-runs and wraps every ~32us once MoodLights_Init() starts it,
        // so its value at this arbitrary moment isn't very predictable run
        // to run - not a real entropy source, just enough that the
        // twinkle pattern isn't identical every single power-up.
        rngState = (uint16_t)(TMR2 + 1u);
        rngSeeded = true;
    }

    if (fireflyTicksRemaining > 0)
    {
        fireflyTicksRemaining--;
        return;
    }

    // Time for a flash: on for a fixed, consistent duration (a real delay,
    // not ticks, so it doesn't depend on how long other main-loop work
    // happens to take)...
    LED_3_SetHigh();
    __delay_ms(FIREFLY_FLASH_MS);
    LED_3_SetLow();

    // ...then a random dark gap before the next one.
    fireflyTicksRemaining = random_range(FIREFLY_OFF_MIN_TICKS, FIREFLY_OFF_MAX_TICKS);
}
