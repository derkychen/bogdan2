#ifndef CONTROLLER_DIGITAL_H
#define CONTROLLER_DIGITAL_H

#include "controller.h"
#include <stdbool.h>

/*
 * Declares digital (controller Trigger IN) functions
 */

void controller_configure_trigger_in(const Controller *controller);

void controller_trigger_in_write_signal(const Controller *controller, bool high);

void controller_pulse_trigger_in(const Controller *controller);

#endif