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
#include <string.h>
#include <stdlib.h>

/*
    Main application
*/

void uart_write(uint8_t data)
{
    while (!EUSART1_IsTxReady())
    {
        // wait until TX1REG can accept another byte
    }

    EUSART1_Write(data);
}

void dfplayer_send(uint8_t command, uint16_t parameter)
{
    uint8_t high = parameter >> 8;
    uint8_t low  = parameter & 0xFF;

    uint16_t checksum =
        0 - (0xFF + 0x06 + command + 0x00 + high + low);

    uart_write(0x7E);
    uart_write(0xFF);
    uart_write(0x06);
    uart_write(command);
    uart_write(0x00);
    uart_write(high);
    uart_write(low);
    uart_write(checksum >> 8);
    uart_write(checksum & 0xFF);
    uart_write(0xEF);
}

// Tags are written as "<name>::<track>", e.g. "FOREST::2". The name is just
// a human-readable label for whoever wrote the tag - all we need is the
// track number, which selects a file in the DFPlayer's /MP3 folder (see
// dfplayer_send(0x12, ...) below).
static uint16_t parse_track_number(const char *text)
{
    const char *sep = strstr(text, "::");
    if (sep == NULL)
    {
        return 0;
    }
    return (uint16_t)atoi(sep + 2);
}

void PN532_Wakeup(void)
{
    static const uint8_t wakeup[] =
    {
        0x55, 0x55,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
        0x05, 0xFB,
        0xD4, 0x14, 0x01, 0x14, 0x00,
        0x03,
        0x00
    };

    for (uint8_t i = 0; i < sizeof(wakeup); i++)
    {
        while (!EUSART2_IsTxReady())
        {
            // Wait until TX register is ready
        }

        EUSART2_Write(wakeup[i]);
    }

    // Wait until the final byte has physically left the UART
    while (!EUSART2_IsTxDone())
    {
    }
}

int main(void)
{
    SYSTEM_Initialize();

    LED_SetDigitalOutput();
    LED_SetHigh();
    __delay_ms(1000);

    
//    PN532_Wakeup();
//    while(1){
//        
//    }
    
    
    
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

    uint8_t lastUid[PN532_UID_MAX_LEN];
    uint8_t lastUidLen = 0;

    while (1)
    {
        uint8_t uid[PN532_UID_MAX_LEN];
        uint8_t uidLen;

        if (PN532_ReadPassiveTarget(uid, &uidLen))
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
                        dfplayer_send(0x12, track);
                    }
                }
                memcpy(lastUid, uid, uidLen);
                lastUidLen = uidLen;
            }
        }
        else
        {
            // Tag pulled away - let the same tag retrigger next time it's tapped.
            lastUidLen = 0;
            LED_SetLow();
        }

        __delay_ms(150);
    }
}
