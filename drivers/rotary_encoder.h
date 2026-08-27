/*
 * rotary_encoder.h - Driver for an EC11 rotary encoder (quadrature VOL_A/
 * VOL_B pins) with an integrated pushbutton (POWER pin), all wired active-low.
 *
 * Interrupt-on-change for VOL_A/VOL_B/POWER is already configured by MCC's
 * pin manager (PIN_MANAGER_Initialize(): both edges on VOL_A/VOL_B,
 * negative edge only on POWER, PIE0bits.IOCIE enabled). This driver hooks
 * into that via the pin manager's *_SetInterruptHandler() callbacks, so
 * quadrature decoding and press detection happen inside the IOC ISR - but
 * the DFPlayer volume command and the power-button callback are only ever
 * invoked from RotaryEncoder_Tasks(), in normal execution context.
 */

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

// Called from RotaryEncoder_Tasks() (never from the ISR) once per
// debounced power-button press.
typedef void (*RotaryEncoder_PowerButtonCallback)(void);

// Enables pull-ups on VOL_A/VOL_B/POWER (in case the EC11 board doesn't
// already have its own), seeds the quadrature decoder with the pins'
// current state, and registers this driver's handlers with the pin
// manager. Call once at startup, after SYSTEM_Initialize().
// onPowerButtonPressed may be NULL if you don't need the callback.
void RotaryEncoder_Init(RotaryEncoder_PowerButtonCallback onPowerButtonPressed);

// Poll this from the main loop as often as convenient. Applies any volume
// change accumulated by the encoder ISR (via DFPlayer_AdjustVolume()) and
// runs the power-button debounce, invoking the registered callback once
// per physical press.
void RotaryEncoder_Tasks(void);

#endif // ROTARY_ENCODER_H
