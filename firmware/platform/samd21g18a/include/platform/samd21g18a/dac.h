#ifndef PLATFORM_SAMD21G18A_DAC_H
#define PLATFORM_SAMD21G18A_DAC_H

#include "platform/samd21g18a/pin.h"
#include <stdint.h>

#define PLATFORM_SAMD21G18A_DAC_BITS      (10u)
#define PLATFORM_SAMD21G18A_DAC_MIN_VALUE (0u)
#define PLATFORM_SAMD21G18A_DAC_MAX_VALUE \
    ((1u << PLATFORM_SAMD21G18A_DAC_BITS) - 1u)

void platform_samd21g18a_dac_init(void);

void platform_samd21g18a_dac_pin_config_input(
    const platform_samd21g18a_pin_dac_t *pin);

void platform_samd21g18a_dac_pin_write(const platform_samd21g18a_pin_dac_t *pin,
                                       uint16_t dac_val);

#endif
