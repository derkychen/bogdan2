#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/pin.h"
#include "sam.h" // IWYU pragma: keep
#include <stddef.h>

void
digital_pin_cfg_set_output (digital_pin_t const *pin)
{
    ASSERT(pin != NULL);
    ASSERT(pin_port_group_valid(pin->port_group));
    ASSERT(pin_number_valid(pin->number));

    // Set the pin to an input first.
    PORT->Group[pin->port_group].DIRCLR.reg = (1u << pin->number);

    // Set the pin as a GPIO and disable the input buffer.
    PORT->Group[pin->port_group].PINCFG[pin->number].reg = 0u;

    // Set the pin level to LOW and direction to output.
    PORT->Group[pin->port_group].OUTCLR.reg = (1u << pin->number);
    PORT->Group[pin->port_group].DIRSET.reg = (1u << pin->number);

    return;
}

/**
 * @brief Configure a pin as a high-impedance GPIO input.
 *
 * Peripheral muxing and internal pull resistors are disabled.
 */
void
digital_pin_cfg_set_input (digital_pin_t const *pin)
{
    ASSERT(pin != NULL);
    ASSERT(pin_port_group_valid(pin->port_group));
    ASSERT(pin_number_valid(pin->number));

    // Set the pin to an input.
    PORT->Group[pin->port_group].DIRCLR.reg = (1u << pin->number);

    // Set the pin as a GPIO with input buffer enabled and no internal pull
    // resistor.
    PORT->Group[pin->port_group].PINCFG[pin->number].reg = PORT_PINCFG_INEN;

    return;
}

void
digital_pin_level_set_low (digital_pin_t const *pin)
{
    ASSERT(pin != NULL);
    ASSERT(pin_port_group_valid(pin->port_group));
    ASSERT(pin_number_valid(pin->number));

    PORT->Group[pin->port_group].OUTCLR.reg = (1u << pin->number);

    return;
}

void
digital_pin_level_set_high (digital_pin_t const *pin)
{
    ASSERT(pin != NULL);
    ASSERT(pin_port_group_valid(pin->port_group));
    ASSERT(pin_number_valid(pin->number));

    PORT->Group[pin->port_group].OUTSET.reg = (1u << pin->number);

    return;
}

digital_level_t
digital_pin_read (digital_pin_t const *pin)
{
    ASSERT(pin != NULL);
    ASSERT(pin_port_group_valid(pin->port_group));
    ASSERT(pin_number_valid(pin->number));

    // Enable input buffer.
    PORT->Group[pin->port_group].PINCFG[pin->number].bit.INEN = 1u;

    // Read pin state.
    return ((PORT->Group[pin->port_group].IN.reg & (1u << pin->number)) != 0u)
               ? DIGITAL_LEVEL_HIGH
               : DIGITAL_LEVEL_LOW;
}
