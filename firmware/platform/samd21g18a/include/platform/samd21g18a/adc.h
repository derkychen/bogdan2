#ifndef PLATFORM_SAMD21G18A_ADC_H
#define PLATFORM_SAMD21G18A_ADC_H

#include "platform/samd21g18a/pin.h"
#include <stdint.h>

#define PLATFORM_SAMD21G18A_ADC_BITS      (12u)
#define PLATFORM_SAMD21G18A_ADC_MIN_VALUE (0u)
#define PLATFORM_SAMD21G18A_ADC_MAX_VALUE \
    ((1u << PLATFORM_SAMD21G18A_ADC_BITS) - 1u)

void platform_samd21g18a_adc_init(void);

void platform_samd21g18a_adc_init(void);

void platform_samd21g18a_adc_pin_config_input(
    const platform_samd21g18a_pin_adc_t *pin);

uint16_t platform_samd21g18a_adc_pin_read(
    const platform_samd21g18a_pin_adc_t *pin);

#endif
