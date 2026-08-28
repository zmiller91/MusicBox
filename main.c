 /*
 * MAIN Generated Driver File
 *
 * @file main.c
 *
 * @defgroup main MAIN
 *
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.0
*/

/*
� [2026] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip
    software and any derivatives exclusively with Microchip products.
    You are responsible for complying with 3rd party license terms
    applicable to your use of 3rd party software (including open source
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.?
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR
    THIS SOFTWARE.
*/
#include "mcc_generated_files/system/system.h"
#include "drivers/pn532.h"
#include "drivers/dfplayer.h"
#include "drivers/rotary_encoder.h"
#include "drivers/mood_lights.h"
#include "drivers/scene.h"
#include <string.h>
#include <stdlib.h>

/*
    Main application
*/

// Tags are written as "<name>::<track>", e.g. "FOREST::2". The name is just
// a human-readable label for whoever wrote the tag - all we need is the
// track number, which selects a file in the DFPlayer's /MP3 folder (see
// DFPlayer_PlayTrack() below).
static uint16_t parse_track_number(const char *text)
{
    const char *sep = strstr(text, "::");
    if (sep == NULL)
    {
        return 0;
    }
    return (uint16_t)atoi(sep + 2);
}

// Last tag seen, so the main loop only plays a track when a *new* tag
// shows up rather than replaying on every poll while one sits on the
// reader. lastUidLen == 0 means "no tag" / "forget the last one".
static uint8_t lastUid[PN532_UID_MAX_LEN];
static uint8_t lastUidLen = 0;
static bool isOn = true;

// Placeholder - wire up whatever "power" should actually do (sleep, mute,
// a MOSFET on the speaker rail, etc.). Runs from RotaryEncoder_Tasks() in
// the main loop, not from the IOC ISR, so it's safe to do real work here.
static void on_power_button_pressed(void)
{
    Scene_Stop();

    // Forget the last tag, so if it's still sitting on the reader, the
    // main loop treats it as newly-arrived and plays it again instead of
    // requiring it to be physically removed and re-tapped.
    memset(lastUid, 0, sizeof(lastUid));
    lastUidLen = 0;
    isOn = !isOn;
}

int main(void)
{
    SYSTEM_Initialize();

    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    LED_SetDigitalOutput();
    LED_SetHigh();
    __delay_ms(1000);

    uint8_t initResult = PN532_Init();
    if (initResult != PN532_INIT_OK)
    {
        // Blink out initResult as a count (blink, blink, ... pause) forever,
        // so the failing step can be read off the LED instead of guessed at.
        // See the PN532_INIT_ERR_* codes in pn532.h for what each count means.
        while (1)
        {
            for (uint8_t i = 0; i < initResult; i++)
            {
                LED_SetHigh();
                __delay_ms(200);
                LED_SetLow();
                __delay_ms(200);
            }
            __delay_ms(1000);
        }
    }
    LED_SetLow();

    DFPlayer_Init();
    RotaryEncoder_Init(on_power_button_pressed);
    MoodLights_Init();

    while (1)
    {
        RotaryEncoder_Tasks();
        MoodLights_Tasks();

        uint8_t uid[PN532_UID_MAX_LEN];
        uint8_t uidLen;

        if (isOn && PN532_ReadPassiveTarget(uid, &uidLen))
        {
            LED_SetHigh();

            bool isNewTag = (uidLen != lastUidLen) || (memcmp(uid, lastUid, uidLen) != 0);
            if (isNewTag)
            {
                char text[32];
                if (PN532_ReadNdefText(text, sizeof(text)))
                {
                    uint16_t track = parse_track_number(text);
                    if (track > 0)
                    {
                        Scene_Start(track);
                    }
                }
                memcpy(lastUid, uid, uidLen);
                lastUidLen = uidLen;
            }
        }
        else
        {
            if (lastUidLen != 0)
            {
                // Tag was just pulled away (not a power-off, which already
                // stopped things abruptly in on_power_button_pressed()) -
                // fade out once, not on every poll while it's gone.
                Scene_Stop();
            }
            // Let the same tag retrigger next time it's tapped.
            lastUidLen = 0;
            LED_SetLow();
        }

        __delay_ms(150);
    }
}
