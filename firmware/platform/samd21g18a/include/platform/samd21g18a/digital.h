#ifndef PLATFORM_SAMD21G18A_DIGITAL_H
#define PLATFORM_SAMD21G18A_DIGITAL_H

#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

void platform_samd21g18a_digital_pin_set_direction_output(
    const platform_samd21g18a_pin_digital_t *pin);

void platform_samd21g18a_digital_pin_set_direction_input(
    const platform_samd21g18a_pin_digital_t *pin);

bool platform_samd21g18a_digital_pin_read(
    const platform_samd21g18a_pin_digital_t *pin);

void platform_samd21g18a_digital_pin_write_low(
    const platform_samd21g18a_pin_digital_t *pin);

void platform_samd21g18a_digital_pin_write_high(
    const platform_samd21g18a_pin_digital_t *pin);

#endif
