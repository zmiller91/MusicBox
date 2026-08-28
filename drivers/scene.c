#include "scene.h"
#include "dfplayer.h"
#include "mood_lights.h"
#include "../mcc_generated_files/system/system.h" // for __delay_ms

#define SCENE_FADE_IN_STEPS   30
#define SCENE_FADE_IN_STEP_MS 50 // 30*50ms = 1.5s end to end

#define SCENE_FADE_OUT_STEPS   20
#define SCENE_FADE_OUT_STEP_MS 50 // 20*50ms = 1s end to end

// Fades LED_PWM_1 alone from off to full - the shared tail end of
// Scene_Idle() and Scene_Return().
static void scene_fade_led1_in(void)
{
    for (uint8_t step = 1; step <= SCENE_FADE_IN_STEPS; step++)
    {
        MoodLights_SetBrightness1(MoodLights_GammaBrightness(step, SCENE_FADE_IN_STEPS));
        __delay_ms(SCENE_FADE_IN_STEP_MS);
    }
}

// Fades LED_PWM_1 alone from full to off - used by Scene_StopIdle().
static void scene_fade_led1_out(void)
{
    for (uint8_t step = SCENE_FADE_IN_STEPS; step > 0; step--)
    {
        MoodLights_SetBrightness1(MoodLights_GammaBrightness((uint8_t)(step - 1), SCENE_FADE_IN_STEPS));
        __delay_ms(SCENE_FADE_IN_STEP_MS);
    }
}

void Scene_Idle(void)
{
    scene_fade_led1_in();
}

void Scene_StopIdle(void)
{
    scene_fade_led1_out();
}

void Scene_Start(uint16_t track)
{
    DFPlayer_PlayTrack(track);

    // Crossfade: LED_PWM_1 walks down from full to off while LED_PWM_2
    // walks up from off to full, using the same step so they move at the
    // same rate in opposite directions.
    for (uint8_t step = 1; step <= SCENE_FADE_IN_STEPS; step++)
    {
        uint8_t remaining = (uint8_t)(SCENE_FADE_IN_STEPS - step);
        MoodLights_SetBrightness1(MoodLights_GammaBrightness(remaining, SCENE_FADE_IN_STEPS));
        MoodLights_SetBrightness2(MoodLights_GammaBrightness(step, SCENE_FADE_IN_STEPS));
        __delay_ms(SCENE_FADE_IN_STEP_MS);
    }

    MoodLights_SetTwinkleEnabled(true);
}

void Scene_Stop(void)
{
    MoodLights_SetTwinkleEnabled(false);

    uint8_t startVolume = DFPlayer_GetVolume();

    // One shared loop stepping both volume and LED_PWM_2 brightness by the
    // same fraction each iteration - see the file header for why this
    // can't just be DFPlayer_FadeOutAndStop() + MoodLights_FadeOut() back
    // to back. LED_PWM_1 is deliberately NOT touched here - this function
    // assumes it's already 0 (the caller only reaches for Scene_Stop()
    // when the *active scene*, not the idle glow, is what needs stopping;
    // see Scene_StopIdle() for the other case). Writing the same ramping
    // value to both would yank whichever LED was off up to near-full
    // brightness on the very first iteration before fading it back down.
    for (uint8_t step = SCENE_FADE_OUT_STEPS; step > 0; step--)
    {
        uint8_t remaining = (uint8_t)(step - 1);

        uint16_t brightness = MoodLights_GammaBrightness(remaining, SCENE_FADE_OUT_STEPS);
        uint8_t volume = (uint8_t)((uint32_t)startVolume * remaining / SCENE_FADE_OUT_STEPS);

        MoodLights_SetBrightness2(brightness);
        DFPlayer_SetVolume(volume, false); // fire-and-forget - one of many rapid steps

        __delay_ms(SCENE_FADE_OUT_STEP_MS);
    }

    DFPlayer_Stop();
    DFPlayer_SetVolume(startVolume, true); // restore for the next track, confirmed
}

void Scene_Return(void)
{
    Scene_Stop();
    scene_fade_led1_in();
}
