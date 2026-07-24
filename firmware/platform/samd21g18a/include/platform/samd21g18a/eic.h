#ifndef PLATFORM_SAMD21G18A_EIC_H
#define PLATFORM_SAMD21G18A_EIC_H

#include "platform/samd21g18a/pin.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stdint.h>

/** @brief Enumeration for interrupt sensing. */
typedef enum
{
    PLATFORM_SAMD21G18A_EIC_SENSE_RISE = 0,
    PLATFORM_SAMD21G18A_EIC_SENSE_FALL,
    PLATFORM_SAMD21G18A_EIC_SENSE_BOTH,
    PLATFORM_SAMD21G18A_EIC_SENSE_HIGH,
    PLATFORM_SAMD21G18A_EIC_SENSE_LOW,

    PLATFORM_SAMD21G18A_EIC_SENSE_COUNT,
} platform_samd21g18a_eic_sense_t;

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

/** @brief Disable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_disable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Enable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_enable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Disable hardware event output on a external interrupt line. */
void platform_samd21g18a_eic_line_event_disable(
    platform_samd21g18a_eic_extint_line_t line);

/** @brief Enable hardware event output on a external interrupt line. */
void platform_samd21g18a_eic_line_event_enable(
    platform_samd21g18a_eic_extint_line_t line);

#endif
