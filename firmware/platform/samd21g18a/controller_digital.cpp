#include "samd21g18a.h"
#include "controller_digital.h"

#define CAPTURE_TRIGGER_PORT_GROUP 0
#define CAPTURE_TRIGGER_PIN        9


/* 
 * Capture refers to a measurement taken
 * Captures could be triggered by:
    * 1. (Physical) AND gate output (from laser pulse and controllers' Trigger OUT)
    * 2. Laser Pulse
 * Functions are able to handle any of these as long as the signal is 
 * wired to the correct pin (CAPTURE_TRIGGER_PIN)
 */

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

void controller_configure_capture_trigger(const Controller *controller)
/*
 * Prepares SAMD21 pin to monitor the signal that causes a capture
 */
{
    // DIRCLR: performs DIR = DIR & ~(1 << pin)
    // Set Capture Trigger pin direction to INPUT
    PORT->Group[controller->capture_trigger_port_group]
        .DIRCLR.reg = (1ul << controller->capture_trigger_pin);
    
    // INEN: Enable input buffer
    PORT->Group[controller->capture_triggert_port_group]
        .PINCFG[controller->capture_trigger_pin].bit.INEN = 1;
    
    // PULLEN: Enable internal pull resistor
    // Prevents pin from floating when controller not actively driving it
    PORT->Group[controller->capture_trigger_port_group]
        .PINCFG[controller->capture_trigger_pin].bit.PULLEN = 1;
    
    // Select Pull Down behaviour (pin default LOW)
    PORT->Group[controller->trigger_out_port_group]
        .OUTCLR.reg = (1ul << controller-capture_trigger_pin);
}

bool controller_read_capture_trigger(const Controller *controller)
/*
 * Reads the entire input register for port 
 * Returns True if controller capture trigger = HIGH
 * Returns False if controller capture trigger = LOW
 */
{
    return (
        PORT->Group[controller->capture_trigger_port_group].IN.reg &
        (1ul << controller->trigger_out_pin)                            // mask for desired pin
    ) != 0;
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

void and_gate_configure_output(void)
/*
 * Sets up GPIO pin that represents the 
 * output of software AND condition
 */
{
    // Config AND pin as OUTPUT
    PORT->Group[CAPTURE_TRIGGER_PORT_GROUP]
        .DIRSET.reg = (1ul << CAPTURE_TRIGGER_PIN);

    // Start with the output LOW
    PORT->Group[CAPTURE_TRIGGER_PORT_GROUP]
        .OUTCLR.reg = (1ul << CAPTURE_TRIGGER_PIN);
}

void and_gate_write_signal(bool high)
/*
 * Allows software to drive output signal HIGH or LOW
 * If argument is true, Pin = HIGH
 * If argument is false, Pin = LOW
 */
{
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