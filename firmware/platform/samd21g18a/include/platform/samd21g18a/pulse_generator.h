#ifndef PLATFORM_SAMD21G18A_PULSE_GENERATOR_H
#define PLATFORM_SAMD21G18A_PULSE_GENERATOR_H

#include "platform/samd21g18a/eic.h"
#include <stdbool.h>
#include <stdint.h>

#define PLATFORM_SAMD21G18A_PULSE_GENERATOR_TCC0_TICKS_PER_USEC (48u)

/** @brief Initialization of the one-shot pulse timer. */
typedef void (*platform_samd21g18a_pulse_generator_init_t)(void);

/** @brief Configuration for the one-shot pulse width. */
typedef void (*platform_samd21g18a_pulse_generator_width_set_t)(
    uint32_t width_ticks);

/** @brief Disable firing of the one-shot pulse through EVSYS. */
typedef void (*platform_samd21g18a_pulse_generator_event_disable_t)(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Disable firing of the one-shot pulse through EVSYS. */
typedef void (*platform_samd21g18a_pulse_generator_event_enable_t)(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Re-triggering of the one-shot pulse. */
typedef void (*platform_samd21g18a_pulse_generator_retrigger_t)(void);

/** @brief Structure for a one-shot pulse generator. */
typedef struct
{
    /** Initialization function. */
    platform_samd21g18a_pulse_generator_init_t init;

    /** Pulse generator disable interrupt configuration. */
    platform_samd21g18a_pulse_generator_event_disable_t event_disable;

    /** Pulse generator enable interrupt configuration. */
    platform_samd21g18a_pulse_generator_event_enable_t event_enable;

    /** Pulse width configuration function. */
    platform_samd21g18a_pulse_generator_width_set_t width_set;

    /** Re-triggering function. */
    platform_samd21g18a_pulse_generator_retrigger_t retrigger;
} platform_samd21g18a_pulse_generator_t;

/**
 * @brief Initialize TCC0 (controls expansion port D7).
 *
 * NOTE: This should only be run after GCLK0 is configured to 48 megaherts.
 */
void platform_samd21g18a_pulse_generator_tcc0_init(void);

void platform_samd21g18a_pulse_generator_tcc0_evsys_configure_t(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Disable TCC0 one-shot pulsing through EVSYS. */
void platform_samd21g18a_pulse_generator_tcc0_event_disable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Enable TCC0 one-shot pulsing through EVSYS. */
void platform_samd21g18a_pulse_generator_tcc0_event_enable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Configure TCC0 one-shot pulse width. */
void platform_samd21g18a_pulse_generator_tcc0_width_set(uint32_t ticks);

/** @brief Re-trigger TCC0's one-shot pulse. */
void platform_samd21g18a_pulse_generator_tcc0_retrigger(void);

#endif
