#include "dfplayer.h"
#include "../mcc_generated_files/system/system.h"

static void dfplayer_uart_write(uint8_t data)
{
    while (!EUSART1_IsTxReady())
    {
        // wait until TX1REG can accept another byte
    }

    EUSART1_Write(data);
}

// If a receive overrun has latched, RC1IF never asserts again until CREN
// is cycled, and EUSART1_Read() only clears OERR from *inside* a read - so
// an unhandled overrun would otherwise deadlock waiting for an ACK that
// can never be detected as "ready" (same EUSART2/PN532 quirk documented in
// drivers/pn532.c). Clear it defensively before every wait.
static void dfplayer_uart_clear_overrun(void)
{
    if (RC1STAbits.OERR)
    {
        RC1STAbits.CREN = 0;
        RC1STAbits.CREN = 1;
    }
}

// Delay between RX polls when nothing's ready yet. At 9600 baud a byte
// takes ~1042us to arrive, and EUSART1's hardware RX buffer is only 2
// bytes deep - a coarser (e.g. millisecond-granularity) poll can leave the
// loop asleep long enough for a second byte to land on top of an unread
// one, setting OERR and losing/corrupting data. 20us keeps each poll well
// under a single byte period.
#define DFPLAYER_RX_POLL_US 20

// The DFPlayer's ACK/response frame is the same 10-byte shape as a command:
// 7E FF 06 CMD 00 PARAM_HI PARAM_LO CHK_HI CHK_LO EF. Waits up to
// timeout_us for the first byte, then collects the rest. Returns true only
// for a well-formed ACK (start/end bytes correct, CMD == DFPLAYER_CMD_ACK).
// A false return isn't necessarily an error - plenty of modules/wiring
// don't have ACK feedback hooked up at all, which just looks like a
// timeout here.
static bool dfplayer_wait_for_ack(uint32_t timeout_us)
{
    dfplayer_uart_clear_overrun();

    uint8_t frame[10];
    uint8_t count = 0;
    while (count < sizeof(frame) && timeout_us)
    {
        if (EUSART1_IsRxReady())
        {
            frame[count++] = EUSART1_Read();
        }
        else
        {
            __delay_us(DFPLAYER_RX_POLL_US);
            timeout_us = (timeout_us > DFPLAYER_RX_POLL_US) ? (timeout_us - DFPLAYER_RX_POLL_US) : 0;
        }
    }

    if (count < sizeof(frame))
    {
        return false; // timed out - no ACK support, not wired for RX, or module busy
    }
    return frame[0] == 0x7E && frame[3] == DFPLAYER_CMD_ACK && frame[9] == 0xEF;
}

bool DFPlayer_SendCommand(uint8_t command, uint16_t parameter, bool waitForAck)
{
    uint8_t high = parameter >> 8;
    uint8_t low  = parameter & 0xFF;
    uint8_t feedback = DFPLAYER_FEEDBACK_ACK;

    uint16_t checksum =
        0 - (0xFF + 0x06 + command + feedback + high + low);

    // Drop any stale bytes (a delayed reply to a previous command, an
    // unsolicited "playback finished" notification, etc.) so they can't be
    // mistaken for this command's ACK.
    while (EUSART1_IsRxReady())
    {
        (void)EUSART1_Read();
    }

    dfplayer_uart_write(0x7E);
    dfplayer_uart_write(0xFF);
    dfplayer_uart_write(0x06);
    dfplayer_uart_write(command);
    dfplayer_uart_write(feedback);
    dfplayer_uart_write(high);
    dfplayer_uart_write(low);
    dfplayer_uart_write(checksum >> 8);
    dfplayer_uart_write(checksum & 0xFF);
    dfplayer_uart_write(0xEF);

    // Make sure the frame has actually left the UART (not just been queued
    // into TX1REG) before we start listening for a reply. This happens
    // regardless of waitForAck - "fire and forget" only ever skips the ACK
    // wait, never the write itself.
    while (!EUSART1_IsTxDone())
    {
    }

    if (!waitForAck)
    {
        return true;
    }
    return dfplayer_wait_for_ack(DFPLAYER_ACK_TIMEOUT_US);
}

static uint8_t currentVolume = DFPLAYER_DEFAULT_VOLUME;

void DFPlayer_Init(void)
{
    DFPlayer_SetVolume(DFPLAYER_DEFAULT_VOLUME, true); // setup - confirm it took
}

void DFPlayer_SetVolume(uint8_t volume, bool waitForAck)
{
    if (volume > DFPLAYER_MAX_VOLUME)
    {
        volume = DFPLAYER_MAX_VOLUME;
    }
    currentVolume = volume;
    DFPlayer_SendCommand(DFPLAYER_CMD_SET_VOLUME, volume, waitForAck);
}

void DFPlayer_AdjustVolume(int8_t delta)
{
    int16_t volume = (int16_t)currentVolume + delta;
    if (volume < 0)
    {
        volume = 0;
    }
    if (volume > DFPLAYER_MAX_VOLUME)
    {
        volume = DFPLAYER_MAX_VOLUME;
    }
    // Fire-and-forget: this is the rotary-encoder path, and waiting on an
    // ACK per tick would make fast turns feel laggy.
    DFPlayer_SetVolume((uint8_t)volume, false);
}

uint8_t DFPlayer_GetVolume(void)
{
    return currentVolume;
}

void DFPlayer_PlayTrack(uint16_t track)
{
    DFPlayer_SendCommand(DFPLAYER_CMD_PLAY_MP3_TRACK, track, true);
}

void DFPlayer_Stop(void)
{
    DFPlayer_SendCommand(DFPLAYER_CMD_STOP, 0, true);
}

// Per-step delay for DFPlayer_FadeOutAndStop(). A plain constant, not a
// computed one - XC8's __delay_ms() has to be given a compile-time
// constant, so the fade's total duration is this times the starting
// volume rather than a fixed total split into N steps.
#define DFPLAYER_FADE_STEP_MS 80

void DFPlayer_FadeOutAndStop(void)
{
    uint8_t startVolume = currentVolume;

    while (currentVolume > 0)
    {
        DFPlayer_SetVolume((uint8_t)(currentVolume - 1), false); // fire-and-forget - one of many rapid steps
        __delay_ms(DFPLAYER_FADE_STEP_MS);
    }

    DFPlayer_Stop();
    DFPlayer_SetVolume(startVolume, true); // reset after the fade - confirm it took
}
