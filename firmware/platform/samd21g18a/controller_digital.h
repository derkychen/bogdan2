#ifndef CONTROLLER_DIGITAL_H
#define CONTROLLER_DIGITAL_H

#include "controller.h"
#include <stdbool.h>

/*
 * Declares digital (controller Trigger IN/OUT) functions
 */

void controller_configure_trigger_in(const Controller *controller)
void controller_configure_capture_trigger(const Controller *controller)

void controller_pulse_trigger_in(const Controller *controller);
bool controller_read_capture_trigger(const Controller *controller);

void and_gate_configure_output(void);
void and_gate_write_signal(bool high)

#endif