#include "controller.h"
#include "controller_analog.h"
#include "controller_digital.h"
#include "controller_capture_trigger_config.h"

/*
 * Define the two physical stage controllers
 *
 * Placeholder pin values 
 */
 Controller x_stage = 
 {
    .trigger_in_port_group = 0,
    .trigger_in_pin = 10,

    .analog_in_port_group = 0,
    .analog_in_pin = 2,

    .analog_out_port_group = 0,
    .analog_out_pin = 4,
    .analog_out_adc_channel = 4
 };

 Controller y_stage = 
 {
    .trigger_in_port_group = 0,
    .trigger_in_pin = 11,

    .analog_in_port_group = 0,
    .analog_in_pin = 3,

    .analog_out_port_group = 0,
    .analog_out_pin = 5,
    .analog_out_adc_channel = 5
 };

 void controller_init(const Controller *controller)
 {    
    controller_configure_trigger_in(controller);
    controller_configure_analog_in(controller);
    controller_configure_analog_out(controller);
 }
