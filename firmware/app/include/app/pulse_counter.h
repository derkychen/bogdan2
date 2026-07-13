#ifndef APP_PULSE_COUNTER_H
#define APP_PULSE_COUNTER_H

#include "platform/samd21g18a/eic.h"
#include <stdint.h>

/** @brief Laser pulse counter. */
typedef struct
{
    /** EIC pin connected to the comparator or laser trigger. */
    platform_samd21g18a_eic_pin_t const *trigger;

    /** The number of pulses detected after the last reset. */
    volatile uint32_t count;
} app_pulse_counter_t;

/** @brief Initialize pulse counter and register interrupt callback. */
void app_pulse_counter_init(app_pulse_counter_t                 *counter,
                            platform_samd21g18a_eic_pin_t const *trigger);

/** @brief Return the pulse count. */
uint32_t app_pulse_counter_get_count(app_pulse_counter_t *counter);

/** @brief Enable interrupts from the trigger. */
void app_pulse_counter_start(app_pulse_counter_t *counter);

/** @brief Disable interrupts from the trigger and reset the count. */
void app_pulse_counter_end(app_pulse_counter_t *counter);

#endif
