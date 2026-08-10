/**
 * @file pin.h
 * @brief SAMD21G18A GPIO general functionality.
 *
 * This module provides utilites for configuring and validating SAMD21G18A GPIO
 * pins.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
#ifndef PLATFORM_SAMD21G18A_PIN_H
#define PLATFORM_SAMD21G18A_PIN_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Pin port groups. */
typedef enum
{
    PIN_PORT_GROUP_A = 0u,
    PIN_PORT_GROUP_B = 1u,
} pin_port_group_t;

/** @brief Pin number. */
typedef uint8_t pin_number_t;

/** @brief Pin peripheral functions. */
typedef enum
{
    PIN_PERIPHERAL_FUNCTION_A = 0u,
    PIN_PERIPHERAL_FUNCTION_B = 1u,
    PIN_PERIPHERAL_FUNCTION_C = 2u,
    PIN_PERIPHERAL_FUNCTION_D = 3u,
    PIN_PERIPHERAL_FUNCTION_E = 4u,
    PIN_PERIPHERAL_FUNCTION_F = 5u,
    PIN_PERIPHERAL_FUNCTION_G = 6u,
    PIN_PERIPHERAL_FUNCTION_H = 7u,
} pin_peripheral_function_t;

/** @brief SAMD21 pin structure for storage of pin data. */
typedef struct
{
    pin_port_group_t port_group; /**< Pin port group. */
    pin_number_t     number;     /**< Pin number. */
} pin_t;

/** @brief Check the validity of a pin port group. */
bool pin_port_group_valid(pin_port_group_t group);

/** @brief Check the validity of a pin number. */
bool pin_number_valid(pin_number_t number);

/** @brief Check the validity of a pin peripheral function. */
bool pin_peripheral_function_valid(pin_peripheral_function_t function);

/** @brief Set a pin's complete configuration. */
void pin_set_cfg(pin_t const *pin,
                 bool         peripheral_muxed,
                 bool         input_enabled,
                 bool         pull_enabled,
                 bool         drive_strong);

/** @brief Set the peripheral function of a pin. */
void pin_set_peripheral_function(pin_t const              *pin,
                                 pin_peripheral_function_t peripheral_function);

/** @brief Disconnect a pin from all peripheral functions and write LOW. */
void pin_output_hold_low(pin_t const *pin);

#endif
