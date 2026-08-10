/**
 * @file digital.h
 * @brief Digital microcontroller I/O.
 *
 * This module provides functionality to control SAMD21G18A GPIOs as digital
 * pins.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 *
 * NOTE: The only actually accessible SAMD21G18A pins on the IND.I/O are located
 *       on the MCU expansion port.
 */
#ifndef PLATFORM_SAMD21G18A_DIGITAL_H
#define PLATFORM_SAMD21G18A_DIGITAL_H

#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief Enumeration of digital levels. */
typedef enum
{
    DIGITAL_LEVEL_LOW = 0,
    DIGITAL_LEVEL_HIGH,
} digital_level_t;

/** @brief Type for digital pins. */
typedef pin_t digital_pin_t;

/**
 * @brief Set the direction of a digital pin to be output.
 *
 * NOTE: This function sets the level of the digital pin to be low.
 */
void digital_pin_cfg_set_output(digital_pin_t const *pin);

/** @brief Set the direction of a digital pin to be input. */
void digital_pin_cfg_set_input(digital_pin_t const *pin);

/** @brief Set a digital pin level to LOW. */
void digital_pin_level_set_low(digital_pin_t const *pin);

/** @brief Set a digital pin level to HIGH. */
void digital_pin_level_set_high(digital_pin_t const *pin);

/** @brief Read the level from a digital pin (LOW or HIGH). */
digital_level_t digital_pin_read(digital_pin_t const *pin);

#endif
