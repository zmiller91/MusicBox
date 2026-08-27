/*
 * dfplayer.h - Minimal driver for the DFPlayer Mini MP3 module over UART.
 *
 * Uses EUSART1 (see drivers/dfplayer.c). The DFPlayer Mini doesn't talk
 * back on this wiring (no RX handling here) - commands are fire-and-forget.
 */

#ifndef DFPLAYER_H
#define DFPLAYER_H

#include <stdint.h>

// DFPlayer Mini command bytes this driver uses.
#define DFPLAYER_CMD_PLAY_MP3_TRACK 0x12 // play track N from the /MP3 folder, N = 0001.mp3, 0002.mp3, ...
#define DFPLAYER_CMD_SET_VOLUME     0x06 // 0-30
#define DFPLAYER_CMD_STOP           0x16

#define DFPLAYER_DEFAULT_VOLUME 8

// Builds and sends one DFPlayer Mini command frame.
void DFPlayer_SendCommand(uint8_t command, uint16_t parameter);

// Sets the module to DFPLAYER_DEFAULT_VOLUME. Call once at startup, after
// giving the module a moment to boot.
void DFPlayer_Init(void);

// Sets playback volume, 0 (silent) - 30 (max).
void DFPlayer_SetVolume(uint8_t volume);

// Convenience wrapper: play track number 'track' from the /MP3 folder.
void DFPlayer_PlayTrack(uint16_t track);

// Stops playback.
void DFPlayer_Stop(void);

#endif // DFPLAYER_H
