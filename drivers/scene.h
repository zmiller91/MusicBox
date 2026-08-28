/*
 * scene.h - Coordinates DFPlayer playback with the mood LEDs so the box's
 * "scene" transitions happen as one cohesive experience rather than each
 * subsystem fading on its own schedule.
 *
 * The stump has two lighting states: LED_PWM_1 is the idle glow ("waiting
 * for an animal"), LED_PWM_2 is the active-scene light ("an animal is
 * here and the story is playing"). Exactly one of them is ever lit at a
 * time, and each function below only ever touches the LED(s) relevant to
 * its own transition - Scene_Stop() assumes LED_PWM_1 is already off and
 * only fades LED_PWM_2, Scene_StopIdle() is its LED_PWM_1-only mirror.
 * Callers are responsible for knowing which state they're leaving (see
 * main.c's sceneActive/idleGlowOn) and calling the matching one - nothing
 * here inspects hardware state to guess.
 *
 *   Scene_Idle()     -> LED_PWM_1 fades up alone. Lid opened, nothing on
 *                       the stump.
 *   Scene_Start()    -> track begins immediately, then LED_PWM_1 fades out
 *                       as LED_PWM_2 fades in together, then the LED_3
 *                       firefly twinkle turns on. An animal was placed.
 *   Scene_Stop()     -> volume/LED_PWM_2 fade out together and stop.
 *                       Ends fully dark/silent. Lid closing, power-off, or
 *                       a track finishing on its own with the animal
 *                       still in place - anywhere the *active scene* is
 *                       what needs stopping.
 *   Scene_Return()   -> Scene_Stop(), then LED_PWM_1 fades back in. The
 *                       animal was taken off the stump but the box is
 *                       still open/powered, so it's visibly ready for the
 *                       next one.
 *   Scene_StopIdle() -> LED_PWM_1 fades down alone. Lid closing or
 *                       power-off while the idle glow (not a scene) was
 *                       showing.
 *
 * All fade timing/step tuning lives here (using
 * MoodLights_SetBrightness1()/2()/GammaBrightness() directly, not
 * MoodLights_FadeIn()/FadeOut()) rather than being split across files.
 * The Start/Stop crossfades in particular have to be built this way -
 * DFPlayer_FadeOutAndStop() and MoodLights_FadeOut() each run their own
 * independent blocking loop, so calling separate fades in sequence would
 * finish one entirely before the other even starts. Driving everything
 * that needs to move *together* from one shared loop needs to live
 * somewhere that already knows about both drivers.
 */

#ifndef SCENE_H
#define SCENE_H

#include <stdint.h>

// Fades LED_PWM_1 up alone (LED_PWM_2 stays off). Call when the lid opens
// and nothing is on the stump.
void Scene_Idle(void);

// Starts playing 'track', crossfading LED_PWM_1 out as LED_PWM_2 fades in,
// then enables the firefly twinkle. Call when an animal is placed on the
// stump (assumes LED_PWM_1 was already up from Scene_Idle()).
void Scene_Start(uint16_t track);

// Turns off the firefly twinkle, fades volume and LED_PWM_2 down together
// (assumes LED_PWM_1 is already off), and stops playback. Use this when
// the *active scene* is what needs stopping: lid closing, power-off, or a
// track finishing on its own with the animal still in place.
void Scene_Stop(void);

// Same as Scene_Stop(), then fades LED_PWM_1 back in. Use this when the
// animal is taken off the stump but the box stays open/powered, so it's
// visibly ready for the next one.
void Scene_Return(void);

// Fades LED_PWM_1 down alone (assumes LED_PWM_2 is already off). Use this
// when the *idle glow*, not a scene, is what needs stopping: the lid
// closes or the power button is pressed while nothing is on the stump.
void Scene_StopIdle(void);

#endif // SCENE_H
