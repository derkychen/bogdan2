#ifndef PLATFORM_SAMD21G18A_EIC_H
#define PLATFORM_SAMD21G18A_EIC_H

#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

// Wrap EIC sense bit macros.
#define PLATFORM_SAMD21G18A_EIC_SENSE_RISE (EIC_CONFIG_SENSE0_RISE_Val)
#define PLATFORM_SAMD21G18A_EIC_SENSE_FALL (EIC_CONFIG_SENSE0_FALL_Val)
#define PLATFORM_SAMD21G18A_EIC_SENSE_BOTH (EIC_CONFIG_SENSE0_BOTH_Val)
#define PLATFORM_SAMD21G18A_EIC_SENSE_HIGH (EIC_CONFIG_SENSE0_HIGH_Val)
#define PLATFORM_SAMD21G18A_EIC_SENSE_LOW  (EIC_CONFIG_SENSE0_LOW_Val)

/**
 * @brief EIC callback format.
 *
 * NOTE: Only the external interrupt line is passed. Context pointers are
 *       implemented at the board layer, since the digital signals used for
 *       current interrupts are received over I2C.
 */
typedef void (*platform_samd21g18a_eic_callback_t)(uint8_t eic_line);

/** @brief EIC line structure used for passing data at runtime. */
typedef struct
{
    /** Pin external interrupt line. */
    uint8_t extint_line;
} platform_samd21g18a_eic_line_t;

/** @brief EIC pin structure for storage of pin data. */
typedef struct
{
    /** External interrupt line wrapper. */
    platform_samd21g18a_eic_line_t const *line;

    /** Pin port group. */
    platform_samd21g18a_pin_t const *pin;

    /** Pin sense (e.g. rising, falling, etc.). */
    uint32_t sense;
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
void platform_samd21g18a_eic_register_callback(
    platform_samd21g18a_eic_line_t const *line,
    platform_samd21g18a_eic_callback_t    callback);

/** @brief Enable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_enable(
    platform_samd21g18a_eic_line_t const *line);

/** @brief Disable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_disable(
    platform_samd21g18a_eic_line_t const *line);

/** @brief Clear interrupt flag on a external interrupt line. */
void platform_samd21g18a_eic_line_clear(
    platform_samd21g18a_eic_line_t const *line);

#endif
