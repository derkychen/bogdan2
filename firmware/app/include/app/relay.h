#ifndef APP_RELAY_H
#define APP_RELAY_H

#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/pulser.h"
#include <stdint.h>

/** @brief Laser pulse counter. */
typedef struct
{
    /** Pulser instance that times the pulse. */
    pulser_t const *out;

    /** EIC pin connected to the laser trigger. */
    eic_pin_t const *in;

    /** The number of pulses detected after the last reset. */
    volatile uint32_t count;
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
void relay_pulser_event_start(relay_t *relay);

/** @brief Disarm the relay's pulser. */
void relay_pulser_event_end(relay_t *relay);

/** @brief Re-trigger the pulser pin. */
void relay_pulser_retrigger(relay_t const *relay);

#endif
