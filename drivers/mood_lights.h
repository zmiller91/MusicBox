/*
 * mood_lights.h - Diorama ambience: two independent PWM "mood" LEDs
 * (LED_PWM_1/LED_PWM_2, on PWM3/PWM4) - LED_PWM_1 is the idle "waiting for
 * an animal" glow, LED_PWM_2 is the "scene active" light, orchestrated
 * together by drivers/scene.c - plus a plain on/off LED (LED_3) that
 * flickers like a firefly.
 *
 * LED_3 has no PWM channel, so its "twinkle" is randomized on/off timing
 * rather than a true brightness fade - a real fade would need either a
 * spare PWM channel or a timer-interrupt-driven software PWM, neither of
 * which exist yet. Fine for one LED; worth revisiting if LED_3 grows into
 * a bank of several.
 */

#ifndef MOOD_LIGHTS_H
#define MOOD_LIGHTS_H

#include <stdint.h>
#include <stdbool.h>

#define MOOD_LIGHTS_BRIGHTNESS_MAX 1023 // 10-bit PWM duty range (matches PWM3/PWM4_LoadDutyValue())

// Zeroes both PWM LEDs and sets up LED_3 as a digital output. Timer2
// (which PWM3/PWM4 use as their time base) is started separately by
// TMR2_Initialize(), already called from SYSTEM_Initialize() - call this
// after that, i.e. after SYSTEM_Initialize().
void MoodLights_Init(void);

// Poll once per main-loop iteration. When enabled (see
// MoodLights_SetTwinkleEnabled()), drives LED_3's random firefly-style
// flicker; otherwise a no-op. Assumes it's called roughly every 100-200ms
// (see MOOD_LIGHTS_TICK_MS in mood_lights.c) - it isn't itself timed
// against a real clock, just ticked once per call.
void MoodLights_Tasks(void);

// Turns LED_3's firefly twinkle on or off - e.g. only while a track is
// actually playing. Disabling it immediately switches LED_3 off rather
// than leaving it stuck mid-glow; starts disabled until called.
void MoodLights_SetTwinkleEnabled(bool enabled);

// Sets LED_PWM_1 or LED_PWM_2 to an explicit brightness (0 -
// MOOD_LIGHTS_BRIGHTNESS_MAX) immediately, no fade/delay. Building blocks
// for MoodLights_FadeIn()/FadeOut() and for external synchronized/
// independent fades (see drivers/scene.c, which steps these alongside
// each other and DFPlayer's volume).
void MoodLights_SetBrightness1(uint16_t brightness);
void MoodLights_SetBrightness2(uint16_t brightness);

// Maps a fade-progress fraction (numerator/denominator, e.g. "step 12 of
// 60") to a brightness value using a cubic perceptual curve rather than a
// linear one - human brightness perception is roughly logarithmic, so a
// LINEAR PWM ramp looks like it reaches "full" within the first 20-30% of
// the ramp and does nothing visible for the rest. Exposed so other fades
// (see drivers/scene.c, which fades brightness and DFPlayer volume
// together) can match MoodLights_FadeIn()/FadeOut()'s curve instead of
// falling back to a linear one.
uint16_t MoodLights_GammaBrightness(uint8_t numerator, uint8_t denominator);

// Blocking fade of LED_PWM_1/LED_PWM_2 from off up to full brightness.
// Call when a track starts.
void MoodLights_FadeIn(void);

// Blocking fade of LED_PWM_1/LED_PWM_2 from wherever they currently are
// down to off. Call when a track stops.
void MoodLights_FadeOut(void);

#endif // MOOD_LIGHTS_H
