#ifndef PLATFORM_SAMD21G18A_EIC_H
#define PLATFORM_SAMD21G18A_EIC_H

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

/** @brief EIC pin structure for storage of pin data. */
typedef struct
{
    /** Pin port group. */
    uint8_t port_group;

    /** Pin number group. */
    uint8_t number;

    /** Pin peripheral function (A for EIC). */
    uint8_t peripheral_function;

    /** Pin external interrupt line. */
    uint8_t eic_line;

    /** Pin sense (e.g. rising, falling, etc.). */
    uint32_t sense;
} platform_samd21g18a_eic_pin_t;

/** @brief Initialize the EIC peripheral. */
void platform_samd21g18a_eic_init(void);

/** @brief Initialize a pin connected to the EIC. */
void platform_samd21g18a_eic_pin_init(platform_samd21g18a_eic_pin_t *pin,
                                      uint8_t                        port_group,
                                      uint8_t                        number,
                                      uint8_t  peripheral_function,
                                      uint8_t  eic_line,
                                      uint32_t sense);

/** @brief Register a callback for a pin that runs on every interrupt. */
void platform_samd21g18a_eic_register_callback(
    const platform_samd21g18a_eic_pin_t *pin,
    platform_samd21g18a_eic_callback_t   callback);

/** @brief Enable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_enable(uint8_t eic_line);

/** @brief Disable interrupts on a external interrupt line. */
void platform_samd21g18a_eic_line_disable(uint8_t eic_line);

/** @brief Clear interrupt flag on a external interrupt line. */
void platform_samd21g18a_eic_line_clear(uint8_t eic_line);

#endif
