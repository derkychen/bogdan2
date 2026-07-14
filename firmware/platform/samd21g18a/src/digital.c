#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/pin.h"
#include "sam.h" // IWYU pragma: keep
#include <stddef.h>

void
platform_samd21g18a_digital_pin_direction_set_output (
    platform_samd21g18a_digital_pin_t const *pin)
{
    PLATFORM_SAMD21G18A_ASSERT(pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(pin->port_group
                               <= PLATFORM_SAMD21G18A_PIN_PORT_GROUP_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(pin->number
                               <= PLATFORM_SAMD21G18A_PIN_NUMBER_COUNT);

    PORT->Group[pin->port_group].DIRSET.reg = (1UL << pin->number);

    return;
}
void
platform_samd21g18a_digital_pin_direction_set_input (
    platform_samd21g18a_digital_pin_t const *pin)
{
    PLATFORM_SAMD21G18A_ASSERT(pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(pin->port_group
                               <= PLATFORM_SAMD21G18A_PIN_PORT_GROUP_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(pin->number
                               <= PLATFORM_SAMD21G18A_PIN_NUMBER_COUNT);

    PORT->Group[pin->port_group].DIRCLR.reg = (1UL << pin->number);

    return;
}

void
platform_samd21g18a_digital_pin_level_set_low (
    platform_samd21g18a_digital_pin_t const *pin)
{
    PLATFORM_SAMD21G18A_ASSERT(pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(pin->port_group
                               <= PLATFORM_SAMD21G18A_PIN_PORT_GROUP_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(pin->number
                               <= PLATFORM_SAMD21G18A_PIN_NUMBER_COUNT);

    PORT->Group[pin->port_group].OUTCLR.reg = (1UL << pin->number);

    return;
}

void
platform_samd21g18a_digital_pin_level_set_high (
    platform_samd21g18a_digital_pin_t const *pin)
{
    PLATFORM_SAMD21G18A_ASSERT(pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(pin->port_group
                               <= PLATFORM_SAMD21G18A_PIN_PORT_GROUP_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(pin->number
                               <= PLATFORM_SAMD21G18A_PIN_NUMBER_COUNT);

    PORT->Group[pin->port_group].OUTSET.reg = (1UL << pin->number);

    return;
}

platform_samd21g18a_digital_level_t
platform_samd21g18a_digital_pin_read (
    platform_samd21g18a_digital_pin_t const *pin)
{
    PLATFORM_SAMD21G18A_ASSERT(pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(pin->port_group
                               <= PLATFORM_SAMD21G18A_PIN_PORT_GROUP_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(pin->number
                               <= PLATFORM_SAMD21G18A_PIN_NUMBER_COUNT);

    // Enable input buffer.
    PORT->Group[pin->port_group].PINCFG[pin->number].bit.INEN = 1;

    // Read pin state.
    return ((PORT->Group[pin->port_group].IN.reg & (1UL << pin->number)) != 0)
               ? PLATFORM_SAMD21G18A_DIGITAL_LEVEL_HIGH
               : PLATFORM_SAMD21G18A_DIGITAL_LEVEL_LOW;
}
