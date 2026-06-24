#include "platform/samd21g18a/adc.h"
#include "platform/samd21g18a/utils.h"
#include "samd21g18a.h"

static void
adc_poll_sync (void)
{
    while (ADC->STATUS.bit.SYNCBUSY)
    {
    }

    return;
}

void
platform_samd21g18a_adc_init (void)
{
    // Enable peripheral bus clock.
    PM->APBCMASK.reg |= PM_APBCMASK_ADC;

    // Turn on ADC clock connection.
    GCLK->CLKCTRL.reg
        = GCLK_CLKCTRL_ID_ADC | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;

    platform_samd21g18a_utils_gclk_poll_sync();

    return;
}

void
platform_samd21g18a_adc_pin_config_input (
    const platform_samd21g18a_pin_adc_t *pin)
{
    uint8_t pmux_index = pin->number / 2u;

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

    PORT->Group[pin->port_group].PINCFG[pin->number].bit.PMUXEN = 1u;

    ADC->CTRLA.bit.SWRST = 1u;
    adc_poll_sync();

    ADC->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC1;

    ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1;

    ADC->SAMPCTRL.reg = 5;

    ADC->INPUTCTRL.reg
        = ADC_INPUTCTRL_MUXPOS(pin->adc_input) | ADC_INPUTCTRL_MUXNEG_GND;

    adc_poll_sync();

    ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV64 | ADC_CTRLB_RESSEL_12BIT;
    adc_poll_sync();

    ADC->CTRLA.bit.ENABLE = 1u;
    adc_poll_sync();
}

uint16_t
platform_samd21g18a_adc_pin_read (const platform_samd21g18a_pin_adc_t *pin)
{
    ADC->INPUTCTRL.bit.MUXPOS = pin->adc_input;
    adc_poll_sync();

    ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;

    ADC->SWTRIG.bit.START = 1u;
    adc_poll_sync();

    while (!ADC->INTFLAG.bit.RESRDY)
    {
    }

    return ADC->RESULT.reg;
}
