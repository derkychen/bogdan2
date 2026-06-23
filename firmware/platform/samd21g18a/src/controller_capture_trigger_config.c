#include "samd21g18a.h"
#include "controller_capture_trigger_config.h"

/* 
 * Capture refers to a measurement taken
 * Captures could be triggered by:
    * 1. (Physical) AND gate output (from laser pulse and controllers' Trigger OUT)
    * 2. Laser Pulse
 * Functions are able to handle any of these as long as the signal is 
 * wired to the correct pin (CAPTURE_TRIGGER_PIN)
 */

static volatile bool capture_trigger_received = false;

bool controller_capture_trigger_received(void)
{
    return capture_trigger_received;
}

void controller_capture_trigger_clear(void)
{
    capture_trigger_received = false;
}
static void controller_configure_capture_trigger_pin(void)
/* 
 * Configure capture trigger pin as EIC input
 * Placeholder: PA03 / EXTINT 2
 */
{
    // Set Capture Trigger pin direction to input
    PORT->Group[CAPTURE_TRIGGER_PORT_GROUP]
        .DIRCLR.reg = (1u << CAPTURE_TRIGGER_PIN);

    // enable switch from GPIO
    PORT->Group[CAPTURE_TRIGGER_PORT_GROUP]
        .PINCFG[CAPTURE_TRIGGER_PIN]
        .bit.PMUXEN = 1;

    // enable read voltage
    PORT->Group[CAPTURE_TRIGGER_PORT_GROUP]
        .PINCFG[CAPTURE_TRIGGER_PIN]
        .bit.INEN = 1;
    
    // routes PA02 to EXTINT2 channel
    PORT->Group[CAPTURE_TRIGGER_PORT_GROUP]
        .PMUX[CAPTURE_TRIGGER_PIN / 2]
        .bit.PMUXE = MUX_PA02A_EIC_EXTINT2;
}

void controller_capture_trigger_init(void)
{
    // Enable EIC bus clock so the CPU can access EIC peripheral
    PM->APBAMASK.reg |= PM_APBAMASK_EIC;
    
    GCLK->CLKCTRL.reg = 
        GCLK_CLKCTRL_ID_EIC |           // Select EIC generic clock channel
        GCLK_CLKCTRL_GEN_GCLK0 |        // Use GCLK0 as the clock source
        GCLK_CLKCTRL_CLKEN;             // Enable the generic clock channel

    while (GCLK->STATUS.bit.SYNCBUSY)
    {
    }

    controller_configure_capture_trigger_pin();

    EIC->CTRL.bit.ENABLE = 0;

    while (EIC->STATUS.bit.SYNCBUSY)
    {
    }

    // Configure to rising edge
    EIC->CONFIG[0].reg &= ~EIC_CONFIG_SENSE2_Mask;
    EIC->CONFIG[0].reg |= EIC_CONFIG_SENSE2_RISE;

        
    // Clear old interrupt flag
    EIC->INTFLAG.reg = CAPTURE_TRIGGER_EXTINT_MASK;

    // Enable interrupts
    EIC->INTENSET.reg = CAPTURE_TRIGGER_EXTINT_MASK;

    NVIC_EnableIRQ(EIC_IRQn);

    // Turn EIC peripheral on
    EIC->CTRL.bit.ENABLE = 1;

    while (EIC->STATUS.bit.SYNCBUSY)
    {
    }

}

bool controller_read_capture_trigger(void)
{
    return (
        PORT->Group[CAPTURE_TRIGGER_PORT_GROUP].IN.reg & 
        (1u << CAPTURE_TRIGGER_PIN)
    ) != 0;
}

void EIC_Handler(void)
{
    if (EIC->INTFLAG.reg & CAPTURE_TRIGGER_EXTINT_MASK)
    {
        EIC->INTFLAG.reg = CAPTURE_TRIGGER_EXTINT_MASK;
        capture_trigger_received = true;
    }
}