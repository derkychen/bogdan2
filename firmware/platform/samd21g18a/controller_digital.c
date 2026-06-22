#include "samd21g18a.h"
#include "controller_digital.h"

static void delay_cycles(volatile uint32_t cycles)
{
    while (cycles--)
    {
    }
}

void controller_configure_trigger_in(const Controller *controller)
/*
 * Prepares SAMD21 GPIO pin to 
  *send trigger pulses to controller (Trigger IN)
 */
{
    // DIRSET: performs DIR = DIR ! (1 << pin)
    // Set Trigger IN pin direction to OUTPUT
    PORT->Group[controller->trigger_in_port_group]
        .DIRSET.reg = (1ul << controller->trigger_in_pin);
    
    // Start with the output LOW
    PORT->Group[controller->trigger_in_port_group]
        .OUTCLR.reg = (1ul << controller->trigger_in_pin)
}

void capture_trigger_in_write_signal(const Controller *controller, bool high)
/*
 * Sets up GPIO pin that represents the 
 * output of software capture trigger condition
 * Allows software to drive output signal HIGH or LOW
 * If argument is true, Pin = HIGH
 * If argument is false, Pin = LOW
 */
{
    // Config capture trigger pin as OUTPUT
    PORT->Group[TRIGGER_IN_PORT_GROUP]
        .DIRSET.reg = (1ul << TRIGGER_IN_PIN);

    // Start with the output LOW
    PORT->Group[TRIGGER_IN_PORT_GROUP]
        .OUTCLR.reg = (1ul << TRIGGER_IN_PIN);

    if (high) {
        // Set pin HIGH
        PORT->Group[CAPTURE_TRIGGER_PORT_GROUP]
            .OUTSET.reg = (1ul << CAPTURE_TRIGGER_PIN);
    } else {

        // OUTCLR: performs OUT = OUT & ~mask
        // Otherwise set pin LOW
        PORT->Group[CAPTURE_TRIGGER_PORT_GROUP]
            .OUTCLR.reg = (1ul << CAPTURE_TRIGGER_PIN);
    }
}

void controller_pulse_trigger_in(const Controller *controller)
/*
 * Generate short TTL pulse for controller's Trigger IN
 */
{
    // OUTSET: performs OUT = OUT | (1 << pin)
    // Starts trigger pulse
    PORT->GROUP[controller->trigger_in_port_group]
        .OUTSET.reg = (1ul << controller->trigger_in_pin)
    
    // Hold pulse HIGH
    delayMicroseconds(10);

    // Drive Trigger IN LOW
    PORT->Group[controller->trigger_in_port_group]
        .OUTCLR.reg = (1ul << controller->trigger_in_pin);
}

