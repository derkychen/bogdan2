#ifndef PULSE_COUNTER_H
#define PULSE_COUNTER_H

#include "platform/samd21g18a/eic.h"
#include <stdint.h>

/** @brief Laser pulse counter. */
typedef struct
{
    /** EIC configuration for the comparator or laser trigger input pin. */
    platform_samd21g18a_eic_cfg_t const *trigger_cfg;

    /** The number of pulses detected after the last reset. */
    volatile uint32_t count;
} app_pulse_counter_t;

/** @brief Initialize pulse counter and register interrupt callback. */
void app_pulse_counter_init(app_pulse_counter_t                 *pulse_counter,
                            platform_samd21g18a_eic_cfg_t const *trigger_cfg);

/** @brief Reset pulse count. */
void app_pulse_counter_reset(app_pulse_counter_t *pulse_counter);

/** @brief Return the pulse count. */
uint32_t app_pulse_counter_get(app_pulse_counter_t *pulse_counter);

#endif
