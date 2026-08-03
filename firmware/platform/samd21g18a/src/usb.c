/**
 * @file usb.c
 * @brief Implementation of USB functionality.
 *
 * NOTE: This overrides the vector table function `USB_Handler`.
 */
#include "platform/samd21g18a/usb.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/pin.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h"  // IWYU pragma: keep
#include "tusb.h" // IWYU pragma: keep
#include <stdbool.h>

void
usb_init (void)
{
    // Enable USB peripheral bus clocks.
    PM->AHBMASK.reg |= PM_AHBMASK_USB;
    PM->APBBMASK.reg |= PM_APBBMASK_USB;

    // Route GCLK0 to USB peripheral. This assumes `SystemInit` configured GCLK0
    // to 48 MHz.
    utils_gclk0_enable(GCLK_CLKCTRL_ID_USB);
    utils_gclk_poll_sync();

    // USB pins (DM: PA24, DP: PA25), peripheral function G.
    PORT->Group[PIN_PORT_GROUP_A].PINCFG[24].reg = PORT_PINCFG_PMUXEN;
    PORT->Group[PIN_PORT_GROUP_A].PINCFG[25].reg = PORT_PINCFG_PMUXEN;

    PORT->Group[PIN_PORT_GROUP_A].PMUX[12].reg
        = PORT_PMUX_PMUXE_G | PORT_PMUX_PMUXO_G;

    // USB QoS.
    USB->DEVICE.QOSCTRL.bit.CQOS = 2u;
    USB->DEVICE.QOSCTRL.bit.DQOS = 2u;

    NVIC_ClearPendingIRQ(USB_IRQn);
    NVIC_SetPriority(USB_IRQn, 0u);
    NVIC_EnableIRQ(USB_IRQn);

    bool initialized = tusb_init();

    ASSERT(initialized);

    tud_connect();

    return;
}

void
usb_task (void)
{
    tud_task();

    return;
}

bool
usb_is_mounted (void)
{
    return tud_mounted();
}

/** @brief Overrides the `USB_Handler` function in the vector table. */
void
USB_Handler (void)
{
    tud_int_handler(0u);

    return;
}
