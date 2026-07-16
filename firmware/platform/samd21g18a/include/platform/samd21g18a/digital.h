#ifndef PLATFORM_SAMD21G18A_DIGITAL_H
#define PLATFORM_SAMD21G18A_DIGITAL_H

#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief Enumeration of digital levels. */
typedef enum
{
    PLATFORM_SAMD21G18A_DIGITAL_LEVEL_LOW  = 0,
    PLATFORM_SAMD21G18A_DIGITAL_LEVEL_HIGH = 1,
} platform_samd21g18a_digital_level_t;

/** @brief Type for digital pins. */
typedef platform_samd21g18a_pin_t platform_samd21g18a_digital_pin_t;

/**
 * @brief Set the direction of a digital pin to be output.
 *
 * NOTE: This function sets the level of the digital pin to be low.
 */
void platform_samd21g18a_digital_pin_cfg_set_output(
    platform_samd21g18a_digital_pin_t const *pin);

/** @brief Set the direction of a digital pin to be input. */
void platform_samd21g18a_digital_pin_cfg_set_input(
    platform_samd21g18a_digital_pin_t const *pin);

/** @brief Set a digital pin level to LOW. */
void platform_samd21g18a_digital_pin_level_set_low(
    platform_samd21g18a_digital_pin_t const *pin);

/** @brief Set a digital pin level to HIGH. */
void platform_samd21g18a_digital_pin_level_set_high(
    platform_samd21g18a_digital_pin_t const *pin);

/** @brief Read the level from a digital pin (LOW or HIGH). */
platform_samd21g18a_digital_level_t platform_samd21g18a_digital_pin_read(
    platform_samd21g18a_digital_pin_t const *pin);

#endif
