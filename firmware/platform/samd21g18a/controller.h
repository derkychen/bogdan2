#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

/* Placeholder stage trigger pins
 
 * Current assumption:
 * X stage Trigger OUT -> PA02 / EXTINT2
 * Y stage Trigger OUT -> PA03 / EXTINT3
*/

#define X_STAGE_PORT_GROUP      0   // 0 = PORTA
#define X_STAGE_PIN             2   // PA02
#define X_STAGE_EXTINT_LINE     2   // EXTINT2

#define Y_STAGE_PORT_GROUP      0
#define Y_STAGE_PIN             3
#define Y_STAGE_EXTINT_LINE     3

#define NUM_EXTINT_LINES        16

#define CONTROLLER_TRIGER_EXTINT_MASK \
    ((1u << X_STAGE_EXTINT_LINE) | (1u << Y_STAGE_EXTINT_LINE))

typedef struct Controller
{
    /* Digital output to controller Trigger IN */
    uint8_t trigger_in_port_group;
    uint8_t trigger_in_pin;

    /* Analog output to controller Analog IN */
    uint8_t analog_in_port_group;
    uint8_t analog_in_pin;

    /* Digital input from controller Trigger OUT */
    uint8_t trigger_out_port_group;
    uint8_t trigger_out_pin;
    uint8_t trigger_out_extint_line;

    /* Analog input from controller Analog OUT */
    uint8_t analog_out_port_group;
    uint8_t analog_out_pin;
    uint8_t analog_out_adc_channel;

    /* Software state */
    volatile bool stage_moving;

} Controller;

extern Controller x_stage;
extern Controller y_stage;

void controller_init(void);
void controller_stage_set_moving(Controller * controller);
bool controller_stage_is_moving(const Controller *controller);

#endif