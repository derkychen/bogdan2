#include "samd21g18a.h"
#include "controller.h"

Controller x_stage;
Controller y_stage;

Controller *controller_from_extint[NUM_EXTINT_LINES] = { 0 };

static void controller_configure_trigger_out_pins(void)
{
    /* 
     * Configure X Trigger OUT pin as EIC input
     * Placeholder:
     * X Trigger OUT: PA03 / EXTINT 2
     */

     // enable switch from GPIO
    PORT->Group[X_STAGE_TRIGGER_OUT_PORT_GROUP]
        .PINCFG[X_STAGE_TRIGGER_OUT_PIN]
        .bit.PMUXEN = 1;

    // enable read voltage
    PORT->Group[X_STAGE_TRIGGER_OUT_PORT_GROUP]
        .PINCFG[X_STAGE_TRIGGER_OUT_PIN]
        .bit.INEN = 1;
    
    // routes PA02 to EXTINT2 channel
    PORT->Group[X_STAGE_TRIGGER_OUT_PORT_GROUP]
        .PMUX[X_STAGE_TRIGGER_OUT_PIN / 2]
        .bit.PMUXE = MUX_PA02A_EIC_EXTINT2;

    /* 
     * Y Trigger OUT: PA03 / EXTINT 2
     */
    
    PORT->Group[Y_STAGE_TRIGGER_OUT_PORT_GROUP]
        .PINCFG[Y_STAGE_TRIGGER_OUT_PIN]
        .bit.PMUXEN = 1;

    PORT->Group[Y_STAGE_TRIGGER_OUT_PORT_GROUP]
        .PINCFG[Y_STAGE_TRIGGER_OUT_PIN]
        .bit.INEN = 1;
    
    PORT->Group[Y_STAGE_TRIGGER_OUT_PORT_GROUP]
        .PMUX[Y_STAGE_TRIGGER_OUT_PIN / 2]
        .bit.PMUXE = MUX_PA03A_EIC_EXTINT3;
}

void controller_init(void)
{
    /*
     * Initializes controller objects
     * Configures EIC interrupts
     */

    x_stage.trigger_in_port_group = X_STAGE_TRIGGER_IN_PORT_GROUP;
    x_stage.trigger_in_pin = X_STAGE_TRIGGER_IN_PIN;
    x_stage.analog_in_port_group = X_STAGE_ANALOG_IN_PORT_GROUP;
    x_stage.analog_in_pin = X_STAGE_ANALOG_IN_PIN;
    x_stage.trigger_out_port_group = X_STAGE_TRIGGER_OUT_PORT_GROUP;
    x_stage.trigger_out_pin = X_STAGE_TRIGGER_OUT_PIN;
    x_stage.trigger_out_extint_line = X_STAGE_TRIGGER_OUT_EXTINT_LINE;
    x_stage.analog_out_adc_channel = X_STAGE_ANALOG_OUT_ADC_CHANNEL;
    x_stage.stage_moving = false;

    y_stage.trigger_in_port_group = Y_STAGE_TRIGGER_IN_PORT_GROUP;
    y_stage.trigger_in_pin = Y_STAGE_TRIGGER_IN_PIN;
    y_stage.analog_in_port_group = Y_STAGE_ANALOG_IN_PORT_GROUP;
    y_stage.analog_in_pin = Y_STAGE_ANALOG_IN_PIN;
    y_stage.trigger_out_port_group = Y_STAGE_TRIGGER_OUT_PORT_GROUP;
    y_stage.trigger_out_pin = Y_STAGE_TRIGGER_OUT_PIN;
    y_stage.trigger_out_extint_line = Y_STAGE_TRIGGER_OUT_EXTINT_LINE;
    y_stage.analog_out_adc_channel = Y_STAGE_ANALOG_OUT_ADC_CHANNEL;
    y_stage.stage_moving = false;

    // Map X EXTINT line to the X controller object
    controller_from_extint[X_STAGE_TRIGGER_OUT_EXTINT_LINE] = &x_stage;

    // Map Y EXTINT line to the X controller object
    controller_from_extint[Y_STAGE_TRIGGER_OUT_EXTINT_LINE] = &y_stage;

    // Enable EIC bus clock so the CPU can access EIC peripheral
    PM->APBAMASK.reg |= PM_APBAMASK_EIC;

    GCLK->CLKCTRL.reg = 
        GCLK_CLKCTRL_ID_EIC |           // Select EIC generic clock channel
        GCLK_CLKCTRL_GEN_GCLK0 |        // Use GCLK0 as the clock source
        GCLK_CLKCTRL_CLKEN;             // Enable the generic clock channel

    while (GCLK->STATUS.bit.SYNCBUSY)
    {
    }

    controller_configure_trigger_out_pins();

    EIC->CTRL.bit.ENABLE = 0;

    while (EIC->STATUS.bit.SYNCBUSY)

    // Configure to rising edge
    EIC->CONFIG[0].reg |=
        EIC_CONFIG_SENSE2_RISE |
        EIC_CONFIG_SENSE3_RISE;

    // Clear old interrupt flag
    EIC->INTFLAG.reg = 
        EIC_INTFLAG_EXTINT2 |
        EIC_INTFLAG_EXTINT3;

    // Enable interrupts
    EIC->INTENSET.reg = 
        EIC_INTENSET_EXTINT2 |
        EIC_INTENSET_EXTINT3;
    
    NVIC_EnableIRQ(EIC_IRQn);

    // Turn EIC peripheral on
    EIC->CTRL.bit.ENABLE = 1;

    while (EIC->STATUS.bit.SYNCBUSY)
    {
    }

}

void controller_set_stage_moving(Controller *controller)
{
    controller->stage_moving = true;
}

bool controller_is_stage_moving(const Controller *controller)
{
    return controller->stage_moving;
}