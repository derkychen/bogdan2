#include "platform/samd21g18a/pin.h"
#include "sam.h" // IWYU pragma: keep

void
platform_samdreg18a_pin_output_hold_low (platform_samd21g18a_pin_t const *pin)
{
    // Keep the pin disconnected from any peripherals.
    PORT->Group[pin->port_group].PINCFG[pin->number].bit.PMUXEN = 0U;

    // Preload output LOW to prevent unintended HIGH intervals.
    PORT->Group[pin->port_group].OUTCLR.reg = (1 << pin->number);
    PORT->Group[pin->port_group].DIRSET.reg = (1 << pin->number);

    return;
}

void
platform_samd21g18a_pin_set_peripheral_function (
    platform_samd21g18a_pin_t const              *pin,
    platform_samd21g18a_pin_peripheral_function_t peripheral_function)
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
}
