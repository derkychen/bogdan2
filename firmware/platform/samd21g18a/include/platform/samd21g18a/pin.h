/* @brief Abstraction of pins.
 *
 * NOTE: This module does not implement any of the functionality pertaining to
 * these pins. It only provides structures for storage of pin configurations.
 */
#ifndef PLATFORM_SAMD21G18A_PIN_H
#define PLATFORM_SAMD21G18A_PIN_H

#include <stdint.h>

typedef enum
{
    PLATFORM_SAMD21G18A_PORT_A = 0u,
    PLATFORM_SAMD21G18A_PORT_B = 1u
} platform_samd21g18a_port_t;

typedef enum
{
    PLATFORM_SAMD21G18A_PMUX_A = 0u,
    PLATFORM_SAMD21G18A_PMUX_B = 1u,
    PLATFORM_SAMD21G18A_PMUX_C = 2u,
    PLATFORM_SAMD21G18A_PMUX_D = 3u,
    PLATFORM_SAMD21G18A_PMUX_E = 4u,
    PLATFORM_SAMD21G18A_PMUX_F = 5u,
    PLATFORM_SAMD21G18A_PMUX_G = 6u,
    PLATFORM_SAMD21G18A_PMUX_H = 7u
} platform_samd21g18a_pmux_t;

typedef struct
{
    uint8_t port_group;
    uint8_t number;
    uint8_t peripheral_function;
    uint8_t adc_input;
} platform_samd21g18a_pin_adc_t;

typedef struct
{
    uint8_t port_group;
    uint8_t number;
    uint8_t peripheral_function;
    uint8_t dac_channel;
} platform_samd21g18a_pin_dac_t;

typedef struct
{
    uint8_t port_group;
    uint8_t number;
} platform_samd21g18a_pin_digital_t;

typedef struct
{
    uint8_t port_group;
    uint8_t number;
    uint8_t peripheral_function;
    uint8_t extint_line;
} platform_samd21g18a_pin_eic_t;

void platform_samd21g18a_pin_adc_init(platform_samd21g18a_pin_adc_t *pin,
                                      uint8_t                        port_group,
                                      uint8_t                        number,
                                      uint8_t peripheral_function);

void platform_samd21g18a_pin_dac_init(platform_samd21g18a_pin_dac_t *pin,
                                      uint8_t                        port_group,
                                      uint8_t                        number,
                                      uint8_t peripheral_function);

void platform_samd21g18a_pin_digital_init(
    platform_samd21g18a_pin_digital_t *pin, uint8_t port_group, uint8_t number);

void platform_samd21g18a_pin_eic_init(platform_samd21g18a_pin_eic_t *pin,
                                      uint8_t                        port_group,
                                      uint8_t                        number,
                                      uint8_t peripheral_function,
                                      uint8_t extint_line);
#endif
