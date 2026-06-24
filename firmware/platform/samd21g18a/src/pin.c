#include "platform/samd21g18a/pin.h"

void
platform_samd21g18a_pin_digital_init (platform_samd21g18a_pin_digital_t *pin,
                                      uint8_t port_group,
                                      uint8_t number)
{
    if (port_group > 1u)
    {
        return;
    }

    if (number > 31u)
    {
        return;
    }

    pin->port_group = port_group;
    pin->number     = number;
}

void
platform_samd21g18a_pin_eic_init (platform_samd21g18a_pin_eic_t *pin,
                                  uint8_t                        port_group,
                                  uint8_t                        number,
                                  uint8_t peripheral_function,
                                  uint8_t extint_line)
{
    if (port_group > 1u)
    {
        return;
    }

    if (number > 31u)
    {
        return;
    }

    if (extint_line >= PLATFORM_SAMD21G18A_EIC_NUM_LINES)
    {
        return;
    }

    pin->port_group          = port_group;
    pin->number              = number;
    pin->peripheral_function = peripheral_function;
    pin->extint_line         = extint_line;
}
