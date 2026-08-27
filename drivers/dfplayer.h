/*
 * dfplayer.h - Minimal driver for the DFPlayer Mini MP3 module over UART.
 *
 * Uses EUSART1 (see drivers/dfplayer.c), TX and RX both. Every command
 * requests the module's ACK (feedback byte set), and the write is always
 * confirmed complete on the wire before DFPlayer_SendCommand() returns -
 * but actually waiting for that ACK is per-call (waitForAck), since not
 * every command needs it and not every module/wiring even has it hooked
 * up. Use waitForAck = true for one-off, meaningful commands (stop, a
 * deliberate volume set) and false for rapid-fire ones (a rotary encoder
 * spinning) where stalling on each ACK would feel laggy.
 */

#ifndef DFPLAYER_H
#define DFPLAYER_H

#include <stdint.h>
#include <stdbool.h>

// DFPlayer Mini command bytes this driver uses.
#define DFPLAYER_CMD_PLAY_MP3_TRACK 0x12 // play track N from the /MP3 folder, N = 0001.mp3, 0002.mp3, ...
#define DFPLAYER_CMD_SET_VOLUME     0x06 // 0-30
#define DFPLAYER_CMD_STOP           0x16
#define DFPLAYER_CMD_ACK            0x41 // sent back by the module, never by us

#define DFPLAYER_FEEDBACK_ACK    0x01   // request byte: ask the module to send DFPLAYER_CMD_ACK back
#define DFPLAYER_ACK_TIMEOUT_US  50000  // how long DFPlayer_SendCommand() waits for that ACK (50ms)

#define DFPLAYER_DEFAULT_VOLUME 8
#define DFPLAYER_MAX_VOLUME     25

// Builds and sends one DFPlayer Mini command frame. Always waits for the
// UART transmission to physically finish before returning ("fire and
// forget" never skips that part - only the ACK wait is optional).
// If waitForAck is true, also waits up to DFPLAYER_ACK_TIMEOUT_US for the
// module's ACK before returning.
// Returns true if waitForAck was false, or a well-formed ACK was seen;
// false only means an ACK was requested but didn't arrive in time - which
// isn't necessarily a failed command (could be a module/wiring without ACK
// support - see the file header).
bool DFPlayer_SendCommand(uint8_t command, uint16_t parameter, bool waitForAck);

// Sets the module to DFPLAYER_DEFAULT_VOLUME and waits for confirmation.
// Call once at startup, after giving the module a moment to boot.
void DFPlayer_Init(void);

// Sets playback volume, 0 (silent) - DFPLAYER_MAX_VOLUME (clamped). Pass
// waitForAck = true for a deliberate/one-off set you want confirmed
// (startup, restoring volume after a fade) and false for rapid-fire
// adjustments (see DFPlayer_AdjustVolume) where waiting would feel laggy.
void DFPlayer_SetVolume(uint8_t volume, bool waitForAck);

// Adjusts playback volume by 'delta' steps (negative to lower), clamped to
// 0 - DFPLAYER_MAX_VOLUME. Meant for a rotary encoder or +/- buttons -
// always fire-and-forget (see DFPlayer_SetVolume) so spinning the knob
// quickly doesn't stall waiting on ACKs.
void DFPlayer_AdjustVolume(int8_t delta);

// Returns the volume this driver last set (i.e. its own tracked state, not
// read back from the module - the DFPlayer Mini doesn't report it here).
uint8_t DFPlayer_GetVolume(void);

// Convenience wrapper: play track number 'track' from the /MP3 folder.
void DFPlayer_PlayTrack(uint16_t track);

// Stops playback immediately, no fade.
void DFPlayer_Stop(void);

// Gently ramps volume down to 0 (one step per DFPLAYER_FADE_STEP_MS - see
// dfplayer.c), then stops and restores the volume that was playing before
// the fade, so the next DFPlayer_PlayTrack() sounds normal again. Blocks
// for the duration of the fade (proportional to the current volume - a
// few hundred ms up to a bit over a second at DFPLAYER_MAX_VOLUME).
void DFPlayer_FadeOutAndStop(void);

#endif // DFPLAYER_H
