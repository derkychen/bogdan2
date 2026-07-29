#ifndef PLATFORM_SAMD21G18A_PIN_H
#define PLATFORM_SAMD21G18A_PIN_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Pin port groups. */
typedef enum
{
    PLATFORM_SAMD21G18A_PIN_PORT_GROUP_A = 0u,
    PLATFORM_SAMD21G18A_PIN_PORT_GROUP_B = 1u,
} platform_samd21g18a_pin_port_group_t;

/** @brief Pin number. */
typedef uint8_t platform_samd21g18a_pin_number_t;

/** @brief Pin peripheral functions. */
typedef enum
{
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_A = 0u,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_B = 1u,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_C = 2u,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_D = 3u,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_E = 4u,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_F = 5u,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_G = 6u,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_H = 7u,
} platform_samd21g18a_pin_peripheral_function_t;

/** @brief SAMD21 pin structure for storage of pin data. */
typedef struct
{
    /** Pin port group. */
    platform_samd21g18a_pin_port_group_t port_group;

    /** Pin number. */
    platform_samd21g18a_pin_number_t number;
} platform_samd21g18a_pin_t;

/** @brief Check the validity of a pin port group. */
bool platform_samd21g18a_pin_port_group_valid(
    platform_samd21g18a_pin_port_group_t group);

/** @brief Check the validity of a pin port group. */
bool platform_samd21g18a_pin_number_valid(
    platform_samd21g18a_pin_number_t number);

/** @brief Check the validity of a pin port group. */
bool platform_samd21g18a_pin_peripheral_function_valid(
    platform_samd21g18a_pin_peripheral_function_t function);

/** @brief Disconnect a pin from all peripheral functions and write LOW. */
void platform_samd21g18a_pin_output_hold_low(
    platform_samd21g18a_pin_t const *pin);

/** @brief Set the peripheral function of a pin. */
void platform_samd21g18a_pin_set_peripheral_function(
    platform_samd21g18a_pin_t const              *pin,
    platform_samd21g18a_pin_peripheral_function_t peripheral_function);

#endif
