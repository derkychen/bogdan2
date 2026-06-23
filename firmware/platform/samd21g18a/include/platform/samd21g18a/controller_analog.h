#ifndef CONTROLLER_ANALOG_H
#define CONTROLLER_ANALOG_H

#include "controller.h"
#include <stdint.h>

/*
 * Declares analog (controller Analog IN/OUT) functions
 */

#define CONTROLLER_DAC_BITS         10
#define CONTROLLER_ADC_BITS         12      // not used

#define CONTROLLER_ADC_RESSEL       ADC_CTRLB_RESSEL_12BIT

#define CONTROLLER_DAC_MAX_VALUE    ((1U << CONTROLLER_DAC_BITS) - 1)
#define CONTROLLER_ADC_MAX_VALUE    ((1U << CONTROLLER_ADC_BITS) - 1)       // not used

void setup_adc_dac_clocks(void);

void controller_setup_dac(const Controller *controller);
void controller_setup_adc(const Controller *controller);

void controller_write_analog_in(const Controller *controller, uint16_t value);
uint16_t controller_read_analog_out(const Controller *controller);

#endif