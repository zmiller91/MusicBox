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

void DFPlayer_SendCommand(uint8_t command, uint16_t parameter)
{
    uint8_t high = parameter >> 8;
    uint8_t low  = parameter & 0xFF;

    uint16_t checksum =
        0 - (0xFF + 0x06 + command + 0x00 + high + low);

    dfplayer_uart_write(0x7E);
    dfplayer_uart_write(0xFF);
    dfplayer_uart_write(0x06);
    dfplayer_uart_write(command);
    dfplayer_uart_write(0x00);
    dfplayer_uart_write(high);
    dfplayer_uart_write(low);
    dfplayer_uart_write(checksum >> 8);
    dfplayer_uart_write(checksum & 0xFF);
    dfplayer_uart_write(0xEF);
}

void DFPlayer_Init(void)
{
    DFPlayer_SetVolume(DFPLAYER_DEFAULT_VOLUME);
}

void DFPlayer_SetVolume(uint8_t volume)
{
    DFPlayer_SendCommand(DFPLAYER_CMD_SET_VOLUME, volume);
}

void DFPlayer_PlayTrack(uint16_t track)
{
    DFPlayer_SendCommand(DFPLAYER_CMD_PLAY_MP3_TRACK, track);
}

void DFPlayer_Stop(void)
{
    DFPlayer_SendCommand(DFPLAYER_CMD_STOP, 0);
}
