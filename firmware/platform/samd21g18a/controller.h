#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>
/*
 * This contains only stage-speciifc signals:
 * - Trigger IN: digital output to tell controller to move
 * - Analog IN: analog command send to controller
 * - Analog OUT: analog position feedback read from controller
 *
 * The capture trigger is shared, not part of this struct
 */

typedef struct Controller
{
    // Digital output to controller Trigger IN
    uint8_t trigger_in_port_group;
    uint8_t trigger_in_pin;

    // Analog output to controller Analog IN 
    uint8_t analog_in_port_group;
    uint8_t analog_in_pin;

    // Analog input from controller Analog OUT 
    uint8_t analog_out_port_group;
    uint8_t analog_out_pin;
    uint8_t analog_out_adc_channel;

} Controller;

extern Controller x_stage;
extern Controller y_stage;

void controller_init(Controller *controller);

#endif