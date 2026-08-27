/*
 * pn532.h - Minimal PN532 driver over UART (HSU) for reading an NDEF "Text"
 * record off an ISO14443A tag (NTAG21x / Mifare Ultralight family).
 *
 * Uses EUSART2 (see drivers/pn532.c) - the PN532 module's mode switch must
 * be set to HSU, not I2C or SPI.
 *
 * Scope is deliberately narrow: this is built for the MusicBox project,
 * where each tag carries a single short NDEF Text record like "FOREST::2"
 * written by a phone app (e.g. NFC Tools). It does not support Mifare
 * Classic, multi-record NDEF messages, or the long-form (3-byte) TLV
 * length encoding.
 */

#ifndef PN532_H
#define PN532_H

#include <stdint.h>
#include <stdbool.h>

#define PN532_UID_MAX_LEN 7

// PN532_Init() return codes.
#define PN532_INIT_OK            0
#define PN532_INIT_ERR_NO_COMMS  1 // wake-up + SAMConfiguration got no ACK/response -
                                   // check TX2/RX2 wiring (crossed: PIC TX2 ->
                                   // PN532 RXD, PIC RX2 <- PN532 TXD), that the
                                   // module's mode switch is set to HSU, and a
                                   // common ground
#define PN532_INIT_ERR_RFCONFIG  2 // RFConfiguration command failed
#define PN532_INIT_ERR_BAD_CHIP  3 // GetFirmwareVersion failed, or IC byte wasn't 0x32

// Wakes the PN532 over UART (fused with the first command - see
// pn532_command()'s withWakeup comment in pn532.c for why that has to be
// one write), puts it in normal (non-virtual-card) mode via
// SAMConfiguration, trims its passive-activation retry count so polling
// stays responsive, then sanity-checks it with GetFirmwareVersion.
//
// EUSART2 must already be initialized (SYSTEM_Initialize() does this).
//
// Call once at startup. Returns PN532_INIT_OK on success, or one of the
// PN532_INIT_ERR_* codes above - which are numbered in the order the
// corresponding step runs, so main.c can turn the value straight into a
// blink count for bench debugging (e.g. blink 1 time, pause, repeat).
uint8_t PN532_Init(void);

// Polls once for an ISO14443A card in range (~100 ms max, see PN532_Init's
// retry configuration). On success, fills uid/uidLen and returns true.
// Returns false if nothing is currently in range - this is the normal,
// expected result when no tag is present, not an error.
bool PN532_ReadPassiveTarget(uint8_t *uid, uint8_t *uidLen);

// Reads user memory off the tag most recently found by
// PN532_ReadPassiveTarget(), locates the NDEF Message TLV, and copies the
// text payload of its first (short-form, well-known-type 'T') record into
// 'out' as a NUL-terminated string. 'outSize' includes room for the NUL.
// Returns false if the tag isn't readable or doesn't contain a matching
// NDEF text record.
bool PN532_ReadNdefText(char *out, uint8_t outSize);

#endif // PN532_H
