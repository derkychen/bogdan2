#ifndef PLATFORM_SAMD21G18A_PIN_H
#define PLATFORM_SAMD21G18A_PIN_H

#include <stdint.h>

/** @brief Pin port groups. */
typedef enum
{
    PLATFORM_SAMD21G18A_PIN_PORT_GROUP_A = 0U,
    PLATFORM_SAMD21G18A_PIN_PORT_GROUP_B = 1U,

    PLATFORM_SAMD21G18A_PIN_PORT_GROUP_COUNT,
} platform_samd21g18a_pin_port_group_t;

/** @brief Pin number. */
typedef uint8_t platform_samd21g18a_pin_number_t;

#define PLATFORM_SAMD21G18A_PIN_NUMBER_COUNT (32U)

/** @brief Pin peripheral functions. */
typedef enum
{
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_A = 0U,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_B = 1U,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_C = 2U,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_D = 3U,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_E = 4U,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_F = 5U,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_G = 6U,
    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_H = 7U,

    PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_COUNT,
} platform_samd21g18a_pin_peripheral_function_t;

/** @brief SAMD21 pin structure for storage of pin data. */
typedef struct
{
    /** Pin port group. */
    platform_samd21g18a_pin_port_group_t port_group;

    /** Pin number. */
    platform_samd21g18a_pin_number_t number;
} platform_samd21g18a_pin_t;

#endif
