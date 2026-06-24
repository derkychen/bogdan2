#include "platform/samd21g18a/digital.h"
#include "samd21g18a.h"

void
platform_samd21g18a_digital_pin_set_direction_output (
    const platform_samd21g18a_pin_digital_t *pin)
{
    PORT->Group[pin->port_group].DIRSET.reg = (1ul << pin->number);
}
void
platform_samd21g18a_digital_pin_set_direction_input (
    const platform_samd21g18a_pin_digital_t *pin)
{
    PORT->Group[pin->port_group].DIRCLR.reg = (1ul << pin->number);
}

bool
platform_samd21g18a_digital_pin_read (
    const platform_samd21g18a_pin_digital_t *pin)
{
    // Enable input buffer.
    PORT->Group[pin->port_group].PINCFG[pin->number].bit.INEN = 1;

    // Read pin state.
    return (PORT->Group[pin->port_group].IN.reg & (1ul << pin->number)) != 0;
}

void
platform_samd21g18a_digital_pin_write_low (
    const platform_samd21g18a_pin_digital_t *pin)
{
    PORT->Group[pin->port_group].OUTCLR.reg = (1ul << pin->number);

    return;
}

void
platform_samd21g18a_digital_pin_write_high (
    const platform_samd21g18a_pin_digital_t *pin)
{
    PORT->Group[pin->port_group].OUTSET.reg = (1ul << pin->number);

    return;
}
