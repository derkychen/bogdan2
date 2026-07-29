#ifndef PLATFORM_SAMD21G18A_EIC_H
#define PLATFORM_SAMD21G18A_EIC_H

#include "platform/samd21g18a/pin.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stdint.h>

/** @brief Type for external interrupt line. */
typedef uint8_t platform_samd21g18a_eic_extint_line_t;

/** @brief Enumeration for interrupt sensing. */
typedef enum
{
    PLATFORM_SAMD21G18A_EIC_SENSE_NONE = EIC_CONFIG_SENSE0_NONE_Val,
    PLATFORM_SAMD21G18A_EIC_SENSE_RISE = EIC_CONFIG_SENSE0_RISE_Val,
    PLATFORM_SAMD21G18A_EIC_SENSE_FALL = EIC_CONFIG_SENSE0_FALL_Val,
    PLATFORM_SAMD21G18A_EIC_SENSE_BOTH = EIC_CONFIG_SENSE0_BOTH_Val,
    PLATFORM_SAMD21G18A_EIC_SENSE_HIGH = EIC_CONFIG_SENSE0_HIGH_Val,
    PLATFORM_SAMD21G18A_EIC_SENSE_LOW  = EIC_CONFIG_SENSE0_LOW_Val,
} platform_samd21g18a_eic_sense_t;

/**
 * @brief EIC callback format.
 *
 * The context pointer is used to pass any relevant information.
 */
typedef void (*platform_samd21g18a_eic_callback_t)(
    platform_samd21g18a_eic_extint_line_t line, void *context);

/** @brief EIC pin data. */
typedef struct
{
    /** Pin. */
    platform_samd21g18a_pin_t const *pin;

    /**
     * External interrupt line.
     *
     * WARNING: This is designated for each pin, it is not arbitrary.
     */
    platform_samd21g18a_eic_extint_line_t line;
} platform_samd21g18a_eic_pin_t;

/** @brief EIC configuration structure. */
typedef struct
{
    /** EIC-specific pin. */
    platform_samd21g18a_eic_pin_t const *eic_pin;

    /** Pin sense (e.g. rising, falling, etc.). */
    platform_samd21g18a_eic_sense_t sense;
} platform_samd21g18a_eic_cfg_t;

/** @brief Check if an external interrupt line is valid. */
bool platform_samd21g18a_eic_extint_line_valid(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Check if a sense value is valid. */
bool platform_samd21g18a_eic_sense_valid(platform_samd21g18a_eic_sense_t sense);

/** @brief Poll the EIC until it is ready. */
void platform_samd21g18a_eic_poll_sync(void);

/** @brief Initialize the EIC peripheral. */
void platform_samd21g18a_eic_init(void);

/**
 * @brief Configure a pin connected to the EIC.
 *
 * NOTE: This function disables the interrupt line specified by the
 *       configuration.
 */
void platform_samd21g18a_eic_configure(
    platform_samd21g18a_eic_cfg_t const *cfg);

/** @brief Register a callback for a pin that runs on every interrupt. */
void platform_samd21g18a_eic_register_callback_entry(
    platform_samd21g18a_eic_extint_line_t line,
    platform_samd21g18a_eic_callback_t    callback,
    void                                 *context);

/** @brief Enable event-based output for an external interrupt line. */
void platform_samd21g18a_eic_event_output_enable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Disable event-based output for an external interrupt line. */
void platform_samd21g18a_eic_event_output_disable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Disable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_disable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Enable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_enable(
    platform_samd21g18a_eic_extint_line_t line);

#endif
