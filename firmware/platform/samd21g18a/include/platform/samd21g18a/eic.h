#ifndef PLATFORM_SAMD21G18A_EIC_H
#define PLATFORM_SAMD21G18A_EIC_H

#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    PLATFORM_SAMD21G18A_EIC_SENSE_RISE = 0,
    PLATFORM_SAMD21G18A_EIC_SENSE_FALL,
    PLATFORM_SAMD21G18A_EIC_SENSE_BOTH,
    PLATFORM_SAMD21G18A_EIC_SENSE_HIGH,
    PLATFORM_SAMD21G18A_EIC_SENSE_LOW
} platform_samd21g18a_eic_sense_t;


typedef void (*platform_samd21g18a_eic_callback_t)(uint8_t extint_line,
                                                   void   *context);

void platform_samd21g18a_eic_init(void);

void platform_samd21g18a_eic_config_pin_sense(
    const platform_samd21g18a_pin_eic_t *pin,
    platform_samd21g18a_eic_sense_t      sense);

bool platform_samd21g18a_eic_register_callback(
    uint8_t                            extint_line,
    platform_samd21g18a_eic_callback_t callback,
    void                              *context);

void platform_samd21g18a_eic_enable_line(uint8_t extint_line);

void platform_samd21g18a_eic_disable_line(uint8_t extint_line);

#endif
