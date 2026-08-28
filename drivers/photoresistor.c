#include "photoresistor.h"
#include "../mcc_generated_files/system/system.h"

uint8_t Photoresistor_ReadDarkness(void)
{
    // ADCC_GetSingleConversion() returns ADRESH:ADRESL left-justified in a
    // 16-bit value (ADFM=left, set in ADCC_Initialize()), so the high byte
    // alone (raw >> 8) is already a clean 0-255 reading regardless of the
    // ADC's actual bit depth.
    uint16_t raw = ADCC_GetSingleConversion(PHOTORESISTOR);
    uint8_t brightness = (uint8_t)(raw >> 8);

    return (uint8_t)(255 - brightness); // invert: higher ADC reading (brighter) -> smaller darkness value
}
