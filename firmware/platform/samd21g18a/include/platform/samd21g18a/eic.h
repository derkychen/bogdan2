#ifndef PLATFORM_SAMD21G18A_EIC_H
#define PLATFORM_SAMD21G18A_EIC_H

#include "platform/samd21g18a/pin.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stdint.h>

// Wrap EIC sense bit macros.
#define PLATFORM_SAMD21G18A_EIC_SENSE_RISE (EIC_CONFIG_SENSE0_RISE_Val)
#define PLATFORM_SAMD21G18A_EIC_SENSE_FALL (EIC_CONFIG_SENSE0_FALL_Val)
#define PLATFORM_SAMD21G18A_EIC_SENSE_BOTH (EIC_CONFIG_SENSE0_BOTH_Val)
#define PLATFORM_SAMD21G18A_EIC_SENSE_HIGH (EIC_CONFIG_SENSE0_HIGH_Val)
#define PLATFORM_SAMD21G18A_EIC_SENSE_LOW  (EIC_CONFIG_SENSE0_LOW_Val)

/** @brief Type for external interrupt line. */
typedef uint32_t platform_samd21g18a_eic_sense_t;

/** @brief Type for external interrupt line. */
typedef uint8_t platform_samd21g18a_eic_extint_line_t;

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

/** @brief Initialize the EIC peripheral. */
void platform_samd21g18a_eic_init(void);

/**
 * @brief Configure a pin connected to the EIC.
 *
 * NOTE: The structure whose pointer is passed to this function should be
 *       initialized beforehand.
 */
void platform_samd21g18a_eic_configure(
    platform_samd21g18a_eic_cfg_t const *cfg);

/** @brief Register a callback for a pin that runs on every interrupt. */
void platform_samd21g18a_eic_register_callback_entry(
    platform_samd21g18a_eic_extint_line_t line,
    platform_samd21g18a_eic_callback_t    callback,
    void                                 *context);

/** @brief Enable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_enable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Disable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_disable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Clear interrupt flag on a external interrupt line. */
void platform_samd21g18a_eic_line_clear(
    platform_samd21g18a_eic_extint_line_t line);

#endif
