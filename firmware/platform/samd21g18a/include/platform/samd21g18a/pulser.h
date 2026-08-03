/**
 * @file pulser.h
 * @brief Firing one-shot pulses.
 *
 * The purpose of this module is to trigger other components. It uses the
 * SAMD21G18A event system as well as onboard hardware timers. It aims to be as
 * generic as possible, but currently only supports a pulser on MCU expansion
 * port D7.
 *
 * NOTE: If the pulser output is HIGH when an event fires or a software
 *       re-trigger is called, the pulse is restarted. If the pulser is used to
 *       trigger components with its rising edge, this restarting of the pulse
 *       will not have the intended effect.
 */
#ifndef PLATFORM_SAMD21G18A_PULSER_H
#define PLATFORM_SAMD21G18A_PULSER_H

#include "platform/samd21g18a/evsys.h"
#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief Type for pulser output. */
typedef pin_t pulser_output_t;

/**
 * @brief Enumerate one-shot pulse generating timers.
 *
 * NOTE: Each pulser must be an independent hardware counter. For example,
 *       different waveform outputs from TCC0 are not independent.
 */
typedef enum
{
    PULSER_TIMER_TCC0 = 0,

    PULSER_TIMER_COUNT,
} pulser_timer_t;

/** @brief Structure for pulsers. */
typedef struct
{
    /** Pulser output pin. */
    pulser_output_t const *output;

    /** Identifer for hardware timer. */
    pulser_timer_t timer;
} pulser_t;

/**
 * @brief Configure a one-shot pulse generator.
 *
 * NOTE: GCLK0 must already be configured to 48 megahertz.
 */
void pulser_configure(pulser_t const *pulser);

/**
 * @brief Configure the pulse width.
 *
 * NOTE: This function must be called while the pulser is stopped.
 */
void pulser_width_set(pulser_t const *pulser, uint32_t width_ticks);

/** @brief Re-trigger a pulse through software. */
void pulser_retrigger(pulser_t const *pulser);

/** @brief Get the EVSYS user corresponding to the pulser's event input. */
evsys_user_t pulser_event_user(pulser_t const *pulser);

#endif
