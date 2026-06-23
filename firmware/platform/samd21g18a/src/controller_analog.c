#include "samd21g18a.h"
#include "controller_analog.h"


static void wait_sync_adc(void)
{
/* Wait until SYNCBUSY becomes 0
 * SYNCBUSY is 1 when register is processing configuration change
 * ADC register writes need time to synchronize
 */
    while (ADC->STATUS.bit.SYNCBUSY) 
    {
    }
}

static void wait_sync_dac(void)
{
/* Wait until SYNCBUSY becomes 0
 * DAC register writes need time to synchronize
 */
    while (DAC->STATUS.bit.SYNCBUSY)
    {
    }
}

void setup_adc_dac_clocks(void)
{
    // Enable peripheral bus clock
    PM->APBCMASK.reg |= PM_APBCMASK_ADC | PM_APBCMASK_DAC;

    // Turn on ADC clock connection
    GCLK->CLKCTRL.reg = 
        GCLK_CLKCTRL_ID_ADC |
        GCLK_CLKCTRL_GEN_GCLK0 |
        GCLK_CLKCTRL_CLKEN;
    
    while (GCLK->STATUS.bit.SYNCBUSY) {
    // wait until generic clock controller finishes updating
    }

    GCLK->CLKCTRL.reg = 
        GCLK_CLKCTRL_ID_DAC |
        GCLK_CLKCTRL_GEN_GCLK0 |
        GCLK_CLKCTRL_CLKEN;
    
    while (GCLK->STATUS.bit.SYNCBUSY) 
    {
    }    
}

/*
 * DAC (Digital to Analog) functions are used to drive
 * controller's Analog IN by converting digital value generated
 * by the microcontroller into a corresponding analog voltage (10-bit value)
 * on a configurd output pin (PA02)
*/

void controller_setup_dac(const Controller *controller)
{
    // Enable switch from GPIO to DAC peripheral
    PORT->Group[controller->analog_in_port_group]
        .PINCFP[controller->analog_in_pin].bit.PMUXEN = 1;

    // Config pin multiplexer
    // Connect DAC peripheral to pin PA02
    PORT->Group[controller->analog_in_port_group]
        .PINCFP[controller->analog_in_pin / 2].bit.PMUXE =
        MUX_PA02B_DAC_VOUT;
    
    // Rest ADC peripheral to default state
    DAC->CTRLA.bit.SWRST = 1;
    wait_sync_dac();

    // Configure DAC operation
    // EOEN enables DAC output driver so voltage appears on VOUT pin
    DAC->CTRLB.reg = DAC_CTRLB_REFSEL_AVCC | DAC_CTRLB_EOEN;
    wait_sync_dac();

    // Enable DAC peripheral
    DAC->CTRLA.bit.ENABLE = 1;
    wait_sync_dac();
    
}

void controller_write_analog_in(const Controller *controller, uint16_t value)
{
    (void)controller;               // Controller pointer not used in function

    if (value > CONTROLLER_DAC_MAX_VALUE){
        value = CONTROLLER_DAC_MAX_VALUE;
    }

    // Write desired DAC value into DAC data register
    DAC->DATA.reg = value;
    wait_sync_dac();
}

/*
 * ADC (Analog to Digital) functions are used to measure
 * controller's Analog OUT voltage by converting analog voltage (12-bit)
 * present on a pin into digital value that the SAMD21 can process
*/

void controller_setup_adc(const Controller *controller)
{
    PORT->Group[controller->analog_out_port_group]
        .PINCFG[controller->analog_out_pin].bit.PMUXEN = 1;

    PORT->Group[controller->analog_out_port_group]
        .PMUX[controller->analog_out_pin / 2].bit.PMUXE =
        MUX_PA0B_ADC_AIN4;
    
    ADC->CTRLA.bit.SWRST = 1;
    wait_sync_adc();

    ADC->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC1;

    // Disable hardware averaging
    ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1;

    // Sample time length
    // Larger value = more charging time for ADC sample capacitor
    ADC->SAMPCTRL.reg = 5;

    // Config ADC input selection
    // Result = selected analog channel relative to GND
    ADC->INPUTCTRL.reg = 
        ADC_INPUTCTRL_MUXPOS(controller->analog_out_adc_channel) |
        ADC_INPUCTRL_MUXNEG_GND;
    wait_sync_adc();

    // Config ADC operating mode
    // PRESCALER_DIV64: ADC clock = ADC source clock / 64
    // RESSEL_12BIT: 12 bit conversion result
    ADC->CTRLB.reg = 
        ADC_CTRLB_PRESCALER_DIV64 |
        CONTROLLER_ADC_RESSEL;
    wait_sync_adc();

    ADC->CTRLA.bit.ENABLE = 1;
    wait_sync_adc();
}

uint16_t controller_read_analog_out(const Controller *controller)
{
    // Select which ADC channel to measure
    ADC->INPUTCTRL.bit.MUXPOS = controller->analog_out_adc_channel;
    wait_sync_adc();

    // Clear previous "result ready" flag
    ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;

    // Start new ADC conversion
    ADC->SWTRIG.bit.START = 1;

    while (!ADC->INTFLAG.bit>RESRDY)
    {
    // Wait until conversion complete
    // RESRDY = 1 when ADC result register conatins new valid result
    }

    return ADC->RESULT.reg;
}
