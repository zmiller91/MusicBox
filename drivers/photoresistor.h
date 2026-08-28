/*
 * photoresistor.h - Reads the ambient-light photoresistor on RC2 (ADC
 * channel PHOTORESISTOR - see mcc_generated_files/adcc, which already
 * owns ADC init/timing; this file just does the read + scaling).
 */

#ifndef PHOTORESISTOR_H
#define PHOTORESISTOR_H

#include <stdint.h>

// Returns the current ambient darkness level: 0 = brightest room light,
// 255 = darkest.
//
// Assumes the photoresistor is the UPPER leg of a voltage divider (LDR
// from VDD to RC2, fixed resistor from RC2 to GND) - more light means
// lower LDR resistance means a HIGHER voltage/ADC reading, which this
// inverts (255 - reading) so bigger output always means darker. If your
// wiring is the other way around (LDR from RC2 to GND instead), drop the
// inversion in photoresistor.c - the raw ADC reading will already
// increase with darkness on its own.
//
// This is a raw linear scaling of the ADC's full range, not a calibrated
// lux measurement - the photoresistor's actual bright/dark swing in your
// enclosure will likely only use part of the 0-255 span. Expect to tune
// this (or add your own min/max calibration) once you can see real
// readings on the bench.
uint8_t Photoresistor_ReadDarkness(void);

#endif // PHOTORESISTOR_H
