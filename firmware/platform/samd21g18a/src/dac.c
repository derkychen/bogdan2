#include "platform/samd21g18a/dac.h"
#include "platform/samd21g18a/utils.h"
#include "samd21g18a.h"

static void
dac_poll_sync (void)
{
    while (DAC->STATUS.bit.SYNCBUSY)
    {
    }

    return;
}

void
platform_samd21g18a_dac_init (void)
{
    PM->APBCMASK.reg |= PM_APBCMASK_DAC;

    GCLK->CLKCTRL.reg
        = GCLK_CLKCTRL_ID_DAC | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;

    platform_samd21g18a_utils_gclk_poll_sync();
}

void
platform_samd21g18a_dac_pin_config_input (
    const platform_samd21g18a_pin_dac_t *pin)
{
    uint8_t pmux_index = pin->number / 2u;

    /*
     * Configure pin mux.
     *
     * SAMD21 DAC output is on PA02 / VOUT.
     * PA02 uses peripheral function B.
     */
    if (pin->number & 1u)
    {
        PORT->Group[pin->port_group].PMUX[pmux_index].bit.PMUXO
            = PORT_PMUX_PMUXO_B_Val;
    }
    else
    {
        PORT->Group[pin->port_group].PMUX[pmux_index].bit.PMUXE
            = PORT_PMUX_PMUXE_B_Val;
    }

    /*
     * Enable switch from GPIO control to peripheral control.
     */
    PORT->Group[pin->port_group].PINCFG[pin->number].bit.PMUXEN = 1u;

    // Reset the DAC peripheral to its default state.
    DAC->CTRLA.bit.SWRST = 1u;
    dac_poll_sync();

    /*
     * Configure DAC operation.
     *
     * REFSEL_AVCC: use analog supply as DAC reference.
     * EOEN: enable external output on VOUT pin.
     */
    DAC->CTRLB.reg = DAC_CTRLB_REFSEL_AVCC | DAC_CTRLB_EOEN;
    dac_poll_sync();

    // Enable DAC peripheral.
    DAC->CTRLA.bit.ENABLE = 1u;
    dac_poll_sync();
}

void
platform_samd21g18a_dac_pin_write (const platform_samd21g18a_pin_dac_t *pin,
                                   uint16_t dac_value)
{
    if (dac_value > PLATFORM_SAMD21G18A_DAC_MAX_VALUE)
    {
        dac_value = PLATFORM_SAMD21G18A_DAC_MIN_VALUE;
    }

    // Write the desired DAC value into DAC data register.
    DAC->DATA.reg = dac_value;
    dac_poll_sync();
}
