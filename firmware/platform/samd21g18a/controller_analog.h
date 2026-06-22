#ifndef CONTROLLER_ANALOG_H
#define CONTROLLER_ANALOG_H

#include "controller.h"
#include <stdint.h>

/*
 * Declares analog (controller Analog IN/OUT) functions
 */

void setup_adc_dac_clocks(void);

void controller_setup_dac(const Controller *controller);
void controller_setup_adc(const Controller *controller);

void controller_write_analog_in(const Controller *controller, uint16_t value);
uint16_t controller_read_analog_out(const Controller *controller);

#endif