#ifndef PLATFORM_SAMD21G18A_PIN_H
#define PLATFORM_SAMD21G18A_PIN_H

#include <stdint.h>

/**
 * @brief SAMD21 pin structure for storage of pin data.
 *
 * NOTE: This structure should only be used for initialization.
 */
typedef struct
{
    /** Pin port group. */
    uint8_t port_group;

    /** Pin number group. */
    uint8_t number;

    /** Pin peripheral function (A for EIC, C for I2C PA16/17). */
    uint8_t peripheral_function;
} platform_samd21g18a_pin_t;

#endif
