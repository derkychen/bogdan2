#ifndef CONTROLLER_CAPTURE_TRIGGER_CONFIG_H
#define CONTROLLER_CAPTURE_TRIGGER_CONFIG_H

#include <stdbool.h> 

/*
 * Shared capture trigger input.
 *
 * Placeholder values for wired pins. 
 */

#define CAPTURE_TRIGGER_PORT_GROUP      0
#define CAPTURE_TRIGGER_PIN             2
#define CAPTURE_TRIGGER_EXTINT_LINE     2
#define CAPTURE_TRIGER_EXTINT_MASK      (1u << CAPTURE_TRIGGER_EXTINT_LINE)

void controller_capture_trigger_init(void);

bool controller_capture_trigger_received(void);
void controller_capture_trigger_clear(void);

bool controller_read_capture_trigger(void);

#endif