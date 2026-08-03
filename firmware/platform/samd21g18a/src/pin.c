#include "platform/samd21g18a/pin.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>

#define PIN_NUMBER_COUNT (32u)

/** @brief Check the validity of a pin port group. */
bool
pin_port_group_valid (pin_port_group_t group)
{
    bool valid;

    switch (group)
    {
        case PIN_PORT_GROUP_A:
        case PIN_PORT_GROUP_B:
            valid = true;
            break;
        default:
            valid = false;
            break;
    }

    return valid;
}

/** @brief Check the validity of a pin port group. */
bool
pin_number_valid (pin_number_t number)
{
    return number < PIN_NUMBER_COUNT;
}

/** @brief Check the validity of a pin port group. */
bool
pin_peripheral_function_valid (pin_peripheral_function_t function)
{
    bool valid;

    switch (function)
    {
        case PIN_PERIPHERAL_FUNCTION_A:
        case PIN_PERIPHERAL_FUNCTION_B:
        case PIN_PERIPHERAL_FUNCTION_C:
        case PIN_PERIPHERAL_FUNCTION_D:
        case PIN_PERIPHERAL_FUNCTION_E:
        case PIN_PERIPHERAL_FUNCTION_F:
        case PIN_PERIPHERAL_FUNCTION_G:
        case PIN_PERIPHERAL_FUNCTION_H:
            valid = true;
            break;
        default:
            valid = false;
            break;
    }

    return valid;
}

void
pin_output_hold_low (pin_t const *pin)
{
    // Keep the pin disconnected from any peripherals.
    PORT->Group[pin->port_group].PINCFG[pin->number].bit.PMUXEN = 0U;

    // Preload output LOW to prevent unintended HIGH intervals.
    PORT->Group[pin->port_group].OUTCLR.reg = (1u << pin->number);
    PORT->Group[pin->port_group].DIRSET.reg = (1u << pin->number);

    return;
}

void
pin_set_peripheral_function (pin_t const              *pin,
                             pin_peripheral_function_t peripheral_function)
{
    uint8_t pmux_index = pin->number / 2u;

    if ((pin->number & 1u) == 0u)
    {
        PORT->Group[pin->port_group].PMUX[pmux_index].bit.PMUXE
            = (uint8_t)(peripheral_function & 0x0Fu);
    }
    else
    {
        PORT->Group[pin->port_group].PMUX[pmux_index].bit.PMUXO
            = (uint8_t)(peripheral_function & 0x0Fu);
    }

    PORT->Group[pin->port_group].PINCFG[pin->number].bit.PMUXEN = 1u;

    return;
}
