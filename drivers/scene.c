#include "scene.h"
#include "dfplayer.h"
#include "mood_lights.h"
#include "../mcc_generated_files/system/system.h" // for __delay_ms

#define SCENE_FADE_IN_STEPS   30
#define SCENE_FADE_IN_STEP_MS 50 // 60*50ms = 3s end to end

#define SCENE_FADE_OUT_STEPS   20
#define SCENE_FADE_OUT_STEP_MS 50 // 40*50ms = 2s end to end

void Scene_Start(uint16_t track)
{
    DFPlayer_PlayTrack(track);

    // Built here rather than just calling MoodLights_FadeIn() so this
    // fade's duration/step feel is tunable in one place alongside
    // Scene_Stop()'s, instead of being split across two files.
    for (uint8_t step = 1; step <= SCENE_FADE_IN_STEPS; step++)
    {
        MoodLights_SetBrightness(MoodLights_GammaBrightness(step, SCENE_FADE_IN_STEPS));
        __delay_ms(SCENE_FADE_IN_STEP_MS);
    }

    MoodLights_SetTwinkleEnabled(true);
}

void Scene_Stop(void)
{
    MoodLights_SetTwinkleEnabled(false);

    uint8_t startVolume = DFPlayer_GetVolume();

    // One shared loop stepping both volume and brightness by the same
    // fraction each iteration - this is the part that can't be built out
    // of DFPlayer_FadeOutAndStop() and MoodLights_FadeOut() alone, since
    // those each run their own independent blocking loop and would fade
    // one all the way out before the other even started.
    for (uint8_t step = SCENE_FADE_OUT_STEPS; step > 0; step--)
    {
        uint8_t remaining = (uint8_t)(step - 1);

        uint16_t brightness = MoodLights_GammaBrightness(remaining, SCENE_FADE_OUT_STEPS);
        uint8_t volume = (uint8_t)((uint32_t)startVolume * remaining / SCENE_FADE_OUT_STEPS);

        MoodLights_SetBrightness(brightness);
        DFPlayer_SetVolume(volume, false); // fire-and-forget - one of many rapid steps

        __delay_ms(SCENE_FADE_OUT_STEP_MS);
    }

    DFPlayer_Stop();
    DFPlayer_SetVolume(startVolume, true); // restore for the next track, confirmed
}
