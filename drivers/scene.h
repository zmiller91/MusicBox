/*
 * scene.h - Coordinates DFPlayer playback with the mood LEDs so a tag's
 * "scene" starts and stops as one cohesive experience rather than each
 * subsystem fading on its own schedule.
 *
 * Start: the track begins immediately, then LED_PWM_1/LED_PWM_2 fade in
 * over SCENE_FADE_IN_STEPS * SCENE_FADE_IN_STEP_MS, then (only once
 * they're fully up) the LED_3 firefly twinkle turns on.
 *
 * Stop: the firefly twinkle turns off immediately, then playback volume
 * and LED_PWM_1/LED_PWM_2 brightness ramp down together over
 * SCENE_FADE_OUT_STEPS * SCENE_FADE_OUT_STEP_MS, ending in
 * DFPlayer_Stop(). Both fades are built locally in scene.c (using
 * MoodLights_SetBrightness()/GammaBrightness() directly, not
 * MoodLights_FadeIn()/FadeOut()) so every timing knob for the experience
 * lives in one place. The fade-out in particular has to work this way -
 * DFPlayer_FadeOutAndStop() and MoodLights_FadeOut() each run their own
 * independent blocking loop, so calling both in sequence would fade one
 * all the way out before the other even starts. Fading them *together*
 * means driving both from one shared loop, which needs to live somewhere
 * that already knows about both drivers.
 */

#ifndef SCENE_H
#define SCENE_H

#include <stdint.h>

// Starts playing 'track' and brings the lights up around it. Blocks for
// the duration of the LED fade-in (see SCENE_FADE_IN_STEPS/STEP_MS in
// scene.c).
void Scene_Start(uint16_t track);

// Turns off the firefly twinkle, then fades volume and LED_PWM_1/
// LED_PWM_2 brightness down together (see SCENE_FADE_OUT_STEPS/STEP_MS in
// scene.c) and stops playback. Blocks for the duration of the fade.
void Scene_Stop(void);

#endif // SCENE_H
