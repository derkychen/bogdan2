/**
 * @file relay.h
 * @brief Tracking and relaying laser trigger pulses.
 *
 * This module configures interrupts and events in order to realize pulse
 * counting and relaying functionality. It does pulse counting through
 * interrupts, but does pulse relaying through hardware timers and the event
 * system, which minimizes jitter for more timing-critical tasks.
 *
 * @note The pulse width defined by this module is limits the frequency of the
 *       laser that can be profiled by modes where the oscilloscope is triggered
 *       on every pulse.
 */
#ifndef APP_RELAY_H
#define APP_RELAY_H

#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/pulser.h"
#include <stdint.h>

/** @brief Laser pulse counter. */
typedef struct
{
    pulser_t const  *out;     /**< Pulser instance that times the pulse. */
    eic_pin_t const *in;      /**< EIC pin connected to the laser trigger. */
    evsys_channel_t  channel; /**< EVSYS channel between the pulser and EIC. */

    volatile uint32_t count; /** The number of pulses counted. */
} relay_t;

/** @brief Initialize pulse tracker. */
void relay_init(relay_t *relay, pulser_t const *out, eic_pin_t const *in);

/** @brief Reset the count and start counting pulses. */
void relay_count_start(relay_t *relay);

/** @brief Return the pulse count. */
uint32_t relay_count_get(relay_t const *relay);

/** @brief Reset the count and stop counting pulses. */
void relay_count_end(relay_t *relay);

/** @brief Arm the relay to fire one-shot pulses through the event system. */
void relay_pulser_start(relay_t *relay);

/** @brief Disarm the relay's pulser. */
void relay_pulser_end(relay_t *relay);

/** @brief Safely disable the relay pulser on movement errors. */
void relay_pulser_abort(relay_t *relay);

/** @brief Re-trigger the pulser pin. */
void relay_pulser_retrigger(relay_t const *relay);

#endif
