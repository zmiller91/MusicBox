#include "pn532.h"
#include "../mcc_generated_files/system/system.h" // for _XTAL_FREQ / __delay_ms
#include <string.h>

#define PN532_PREAMBLE    0x00
#define PN532_STARTCODE1  0x00
#define PN532_STARTCODE2  0xFF
#define PN532_POSTAMBLE   0x00
#define PN532_HOSTTOPN532 0xD4
#define PN532_PN532TOHOST 0xD5

#define PN532_CMD_GETFIRMWAREVERSION  0x02
#define PN532_CMD_RFCONFIGURATION     0x32
#define PN532_CMD_SAMCONFIGURATION    0x14
#define PN532_CMD_INLISTPASSIVETARGET 0x4A
#define PN532_CMD_INDATAEXCHANGE      0x40

#define NTAG_CMD_READ 0x30

// Cap on a response frame's LEN field (TFI + response cmd + params). Big
// enough for everything this driver sends (InListPassiveTarget's UID
// response and the 16-byte NTAG READ are the largest).
#define PN532_FRAME_MAX 40

// ---------------------------------------------------------------------
// UART (HSU) transport on EUSART2. EUSART2_Write()/Read() are simple
// polled register accessors (no ISR involved), so these wrappers just spin
// on the TX/RX ready flags directly.
// ---------------------------------------------------------------------

static void pn532_uart_write(const uint8_t *data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
    {
        while (!EUSART2_IsTxReady())
        {
        }
        EUSART2_Write(data[i]);
    }
    while (!EUSART2_IsTxDone())
    {
    }
}

// If a receive overrun has latched, RC2IF (what EUSART2_IsRxReady() checks)
// never asserts again until CREN is cycled - and EUSART2_Read() only clears
// OERR from *inside* a read, so a stuck overrun would otherwise deadlock:
// nothing we send would ever produce a "ready" byte to read, which looks
// identical to the PN532 simply not responding. Clear it defensively before
// every transaction.
static void pn532_uart_clear_overrun(void)
{
    if (RC2STAbits.OERR)
    {
        RC2STAbits.CREN = 0;
        RC2STAbits.CREN = 1;
    }
}



static uint8_t pn532_send_and_capture(
    const uint8_t *packet,
    uint8_t packetLen,
    uint8_t *buffer,
    uint8_t bufferSize,
    uint16_t firstByteTimeoutMs)
{
    uint8_t count = 0;
    uint16_t idleCounter = 0;
    uint16_t firstByteCounter = 0;
    bool receivedAnything = false;

    // Clear any existing overrun
    if (RC2STAbits.OERR)
    {
        RC2STAbits.CREN = 0;
        RC2STAbits.CREN = 1;
    }

    // Flush stale bytes
    while (EUSART2_IsRxReady())
    {
        (void)EUSART2_Read();
    }

    // Send command
    for (uint8_t i = 0; i < packetLen; i++)
    {
        while (!EUSART2_IsTxReady())
        {
        }

        EUSART2_Write(packet[i]);
    }
    
    while (!EUSART2_IsTxDone())
    {
    }

    // Immediately begin servicing RX
    while (count < bufferSize)
    {
        if (EUSART2_IsRxReady())
        {
            buffer[count++] = EUSART2_Read();
            receivedAnything = true;
            idleCounter = 0;
        }
        else
        {
            __delay_us(10);

            if (!receivedAnything)
            {
                firstByteCounter++;

                if (firstByteCounter >= 10000) // ~100 ms
                {
                    break;
                }
            }
            else
            {
                idleCounter++;

                if (idleCounter >= 10000) // ~100 ms idle
                {
                    break;
                }
            }
        }
    }

    return count;
}

static bool pn532_transact(
    uint8_t cmd,
    const uint8_t *params,
    uint8_t paramLen,
    bool withWakeup,
    uint8_t *out,
    uint8_t outMax,
    uint8_t *outLen)
{
    if (paramLen > 7)
    {
        return false;
    }

    // -----------------------------
    // Build command frame
    // -----------------------------

    uint8_t packet[21];
    uint8_t packetLen = 0;

    uint8_t dataLen = (uint8_t)(paramLen + 2);

    if (withWakeup)
    {
        packet[packetLen++] = 0x55;
        packet[packetLen++] = 0x55;
        packet[packetLen++] = 0x00;
        packet[packetLen++] = 0x00;
        packet[packetLen++] = 0x00;
    }

    packet[packetLen++] = PN532_PREAMBLE;
    packet[packetLen++] = PN532_STARTCODE1;
    packet[packetLen++] = PN532_STARTCODE2;

    packet[packetLen++] = dataLen;
    packet[packetLen++] = (uint8_t)(0 - dataLen);

    uint8_t sum = PN532_HOSTTOPN532;

    packet[packetLen++] = PN532_HOSTTOPN532;

    packet[packetLen++] = cmd;
    sum = (uint8_t)(sum + cmd);

    for (uint8_t i = 0; i < paramLen; i++)
    {
        packet[packetLen++] = params[i];
        sum = (uint8_t)(sum + params[i]);
    }

    packet[packetLen++] = (uint8_t)(0 - sum);
    packet[packetLen++] = PN532_POSTAMBLE;

    // -----------------------------
    // Send + capture whole exchange
    // -----------------------------

    uint8_t buffer[32];

    uint8_t count = pn532_send_and_capture(
        packet,
        packetLen,
        buffer,
        sizeof(buffer),
        1000
    );

    // -----------------------------
    // Validate ACK
    // -----------------------------

    static const uint8_t expectedAck[6] =
    {
        0x00, 0x00, 0xFF,
        0x00, 0xFF, 0x00
    };

    if (count < 6)
    {
        return false;
    }

    if (memcmp(buffer, expectedAck, 6) != 0)
    {
        return false;
    }

    // -----------------------------
    // Find response frame
    // -----------------------------

    uint8_t start = 6;

    while ((uint8_t)(start + 2) < count)
    {
        if (buffer[start]     == PN532_PREAMBLE &&
            buffer[start + 1] == PN532_STARTCODE1 &&
            buffer[start + 2] == PN532_STARTCODE2)
        {
            break;
        }

        start++;
    }

    if ((uint8_t)(start + 2) >= count)
    {
        return false;
    }

    // Need at least:
    //
    // 00 00 FF LEN LCS
    //
    if ((uint8_t)(start + 5) > count)
    {
        return false;
    }

    uint8_t len = buffer[start + 3];
    uint8_t lcs = buffer[start + 4];

    if ((uint8_t)(len + lcs) != 0)
    {
        return false;
    }

    if (len < 2)
    {
        return false;
    }

    // Total frame size:
    //
    // 00 00 FF = 3
    // LEN      = 1
    // LCS      = 1
    // DATA     = len
    // DCS      = 1
    // POST     = 1
    //
    // total = len + 7

    uint16_t frameSize = (uint16_t)len + 7;

    if ((uint16_t)start + frameSize > count)
    {
        return false;
    }

    uint8_t dataStart = (uint8_t)(start + 5);

    // -----------------------------
    // Validate TFI + response cmd
    // -----------------------------

    if (buffer[dataStart] != PN532_PN532TOHOST)
    {
        return false;
    }

    if (buffer[dataStart + 1] != (uint8_t)(cmd + 1))
    {
        return false;
    }

    // -----------------------------
    // Validate DCS
    // -----------------------------

    uint8_t responseSum = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        responseSum =
            (uint8_t)(responseSum + buffer[dataStart + i]);
    }

    uint8_t dcsIndex =
        (uint8_t)(dataStart + len);

    if ((uint8_t)(responseSum + buffer[dcsIndex]) != 0)
    {
        return false;
    }

    // -----------------------------
    // Validate postamble
    // -----------------------------

    uint8_t postambleIndex =
        (uint8_t)(dcsIndex + 1);

    if (buffer[postambleIndex] != PN532_POSTAMBLE)
    {
        return false;
    }

    // -----------------------------
    // Copy response parameters
    // -----------------------------

    uint8_t paramOutLen =
        (uint8_t)(len - 2);

    if (paramOutLen > outMax)
    {
        paramOutLen = outMax;
    }

    for (uint8_t i = 0; i < paramOutLen; i++)
    {
        out[i] = buffer[dataStart + 2 + i];
    }

    *outLen = paramOutLen;

    return true;
}

// ---------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------

uint8_t PN532_Init(void)
{
    // EUSART2 itself is already initialized by SYSTEM_Initialize().
    __delay_ms(10); // let the module finish booting after power-up

    uint8_t rsp[16];
    uint8_t rlen;

    // Normal mode, no IRQ pin wired (we poll instead). This has to be the
    // very first command, fused with the wake-up preamble in one write -
    // see pn532_transact()'s withWakeup comment for why.
    uint8_t samcfg[3] = {0x01, 0x14, 0x00};
    if (!pn532_transact(PN532_CMD_SAMCONFIGURATION, samcfg, sizeof(samcfg), true, rsp, sizeof(rsp), &rlen))
    {
        return PN532_INIT_ERR_NO_COMMS; // no ACK / no valid response at all
    }

    // CfgItem 0x05 (MaxRetries): keep the ATR/PSL defaults, but trim passive
    // activation retries so InListPassiveTarget returns quickly when no tag
    // is present instead of blocking for a long time inside the PN532.
    uint8_t rfcfg[4] = {0x05, 0xFF, 0x01, 0x02};
    if (!pn532_transact(PN532_CMD_RFCONFIGURATION, rfcfg, sizeof(rfcfg), false, rsp, sizeof(rsp), &rlen))
    {
        return PN532_INIT_ERR_RFCONFIG;
    }

    // Sanity check that we're actually talking to a PN532, now that comms
    // are confirmed up via the two commands above.
    if (!pn532_transact(PN532_CMD_GETFIRMWAREVERSION, NULL, 0, false, rsp, sizeof(rsp), &rlen) ||
        rlen < 1 || rsp[0] != 0x32)
    {
        return PN532_INIT_ERR_BAD_CHIP;
    }

    return PN532_INIT_OK;
}

bool PN532_ReadPassiveTarget(uint8_t *uid, uint8_t *uidLen)
{
    uint8_t params[2] = {0x01, 0x00}; // MaxTg=1, BrTy=106 kbps type A
    uint8_t rsp[24];
    uint8_t rlen;

    if (!pn532_transact(PN532_CMD_INLISTPASSIVETARGET, params, sizeof(params), false, rsp, sizeof(rsp), &rlen))
    {
        return false;
    }
    if (rlen < 1 || rsp[0] == 0)
    {
        return false; // NbTg == 0: nothing in range right now
    }

    // rsp layout: NbTg, Tg, SENS_RES(2), SEL_RES, NFCIDLen, NFCID[NFCIDLen], ...
    uint8_t nfcidLen = rsp[5];
    if (nfcidLen == 0 || nfcidLen > PN532_UID_MAX_LEN || (uint8_t)(6 + nfcidLen) > rlen)
    {
        return false;
    }

    for (uint8_t i = 0; i < nfcidLen; i++)
    {
        uid[i] = rsp[6 + i];
    }
    *uidLen = nfcidLen;
    return true;
}

// Reads 4 pages (16 bytes) starting at 'page' via InDataExchange + a raw
// NTAG/Ultralight READ command.
static bool ntag_read4pages(uint8_t page, uint8_t *out16)
{
    uint8_t params[3] = {0x01, NTAG_CMD_READ, page}; // Tg=1
    uint8_t rsp[20];
    uint8_t rlen;

    if (!pn532_transact(PN532_CMD_INDATAEXCHANGE, params, sizeof(params), false, rsp, sizeof(rsp), &rlen))
    {
        return false;
    }
    if (rlen < 17 || rsp[0] != 0x00) // rsp[0] is the InDataExchange status byte, 0x00 = OK
    {
        return false;
    }

    memcpy(out16, &rsp[1], 16);
    return true;
}

// Parses a single short, well-known-type ('T') NDEF record's text payload.
static bool parse_ndef_text_record(const uint8_t *rec, uint8_t recLen, char *out, uint8_t outSize)
{
    if (recLen < 3)
    {
        return false;
    }

    uint8_t header = rec[0];
    uint8_t tnf = header & 0x07;
    bool shortRecord = (header & 0x10) != 0;
    bool hasId = (header & 0x08) != 0;

    uint8_t idx = 1;
    uint8_t typeLen = rec[idx++];

    uint16_t payloadLen;
    if (shortRecord)
    {
        if (idx >= recLen)
        {
            return false;
        }
        payloadLen = rec[idx++];
    }
    else if ((uint16_t)(idx + 4) <= recLen)
    {
        // Long-form (4-byte) payload length - unusual for a short tag text,
        // but handle it since it costs little.
        payloadLen = ((uint16_t)rec[idx + 2] << 8) | rec[idx + 3];
        idx += 4;
    }
    else
    {
        return false;
    }

    uint8_t idLen = 0;
    if (hasId)
    {
        if (idx >= recLen)
        {
            return false;
        }
        idLen = rec[idx++];
    }

    if ((uint16_t)(idx + typeLen) > recLen)
    {
        return false;
    }
    bool isText = (tnf == 0x01) && (typeLen == 1) && (rec[idx] == 'T');
    idx = (uint8_t)(idx + typeLen + idLen);

    if (!isText)
    {
        return false;
    }
    if ((uint16_t)idx + payloadLen > recLen)
    {
        return false;
    }

    const uint8_t *payload = &rec[idx];
    uint8_t status = payload[0];
    uint8_t langLen = status & 0x3F; // bit 7 is the UTF-8/UTF-16 flag, ignored (we assume UTF-8/ASCII)
    if ((uint16_t)(1 + langLen) > payloadLen)
    {
        return false;
    }

    uint16_t textLen = (uint16_t)(payloadLen - 1 - langLen);
    if (textLen > (uint16_t)(outSize - 1))
    {
        textLen = (uint16_t)(outSize - 1);
    }

    memcpy(out, &payload[1 + langLen], textLen);
    out[textLen] = '\0';
    return true;
}

bool PN532_ReadNdefText(char *out, uint8_t outSize)
{
    static uint8_t buf[64]; // accumulated user memory, starting at page 4
    uint8_t have = 0;
    uint8_t page = 4;
    uint8_t pos = 0; // TLV walk position within buf[0..have) - persists across page reads

    // NTAG213 has 144 bytes of user memory (pages 4-39); cap the scan there.
    while (page <= 40 && (uint16_t)(have + 16) <= sizeof(buf))
    {
        if (!ntag_read4pages(page, &buf[have]))
        {
            return false;
        }
        have = (uint8_t)(have + 16);
        page = (uint8_t)(page + 4);

        // Walk the TLV area from wherever the last pass left off. A Lock
        // Control TLV (0x01), Memory Control TLV (0x02), or a proprietary
        // one can legitimately sit before the NDEF Message TLV (0x03), so
        // every non-NULL, non-terminator TLV has to be skipped by its own
        // length rather than assumed to be zero padding.
        while (pos < have)
        {
            uint8_t type = buf[pos];

            if (type == 0x00) // NULL TLV: single byte, no length/value
            {
                pos++;
                continue;
            }
            if (type == 0xFE) // Terminator TLV: end of the TLV area
            {
                return false; // reached the end without finding an NDEF message
            }

            if ((uint8_t)(pos + 1) >= have)
            {
                goto need_more; // length byte not buffered yet
            }
            uint8_t len = buf[pos + 1];
            if (len == 0xFF)
            {
                return false; // 3-byte extended length form not supported
            }
            uint8_t valueStart = (uint8_t)(pos + 2);
            if ((uint16_t)valueStart + len > have)
            {
                goto need_more; // value not fully buffered yet
            }

            if (type == 0x03) // NDEF Message TLV
            {
                return parse_ndef_text_record(&buf[valueStart], len, out, outSize);
            }

            pos = (uint8_t)(valueStart + len); // skip any other TLV type
        }
    need_more:;
    }

    return false; // ran out of scan range without a complete, parseable record
}
