/**
 * @file controller.c
 * @brief Implementation of the controller I/O and state layer.
 *
 * NOTE: The usage of "in" and "out" when referring to the controller I/O is
 *       relative to the controller. For example, Trigger OUT is received by the
 *       microcontroller, while Analog IN outputted from the microcontroller.
 */
#include "app/controller.h"
#include "board/indio/analog_output.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"
#include <stddef.h>

// DEBUG:
#include "app/serial.h"

#define START_MOVE_PULSE_WIDTH_MS (1u)

static void trigger_out_isr(eic_extint_line_t line, void *context);

void
controller_init (controller_t                  *controller,
                 pin_t const                   *trigger_in,
                 eic_pin_t const               *trigger_out,
                 analog_output_channel_t const *analog_in)
{
    ASSERT(controller != NULL);
    ASSERT(trigger_in != NULL);
    ASSERT(trigger_out != NULL);
    ASSERT(analog_in != NULL);

    controller->trigger_in   = trigger_in;
    controller->trigger_out  = trigger_out;
    controller->analog_in    = analog_in;
    controller->stage_moving = false;

    digital_pin_cfg_set_output(trigger_in);
    digital_pin_level_set_low(trigger_in);

    eic_configure(trigger_out, EIC_SENSE_RISE);
    eic_register_callback_entry(trigger_out->line, trigger_out_isr, controller);
    eic_interrupt_disable(controller->trigger_out->line);

    if (analog_output_write(analog_in, 0u) == ANALOG_OUTPUT_STATUS_OK)
    {
        // DEBUG:
        serial_write_line("analog output ok");
    }

    // DEBUG:
    serial_write_line("analog output err");
}

bool
controller_get_stage_moving (controller_t const *controller)
{
    ASSERT(controller != NULL);

    return controller->stage_moving;
}

void
controller_set_stage_moving (controller_t *controller, bool stage_moving)
{
    ASSERT(controller != NULL);

    // NOTE: This value will be changed to `false` upon an interrupt.
    controller->stage_moving = stage_moving;

    return;
}

void
controller_interrupt_disable (controller_t const *controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->trigger_out != NULL);

    eic_interrupt_disable(controller->trigger_out->line);

    return;
}

void
controller_interrupt_enable (controller_t const *controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->trigger_out != NULL);

    eic_interrupt_enable(controller->trigger_out->line);

    return;
}

void
controller_pulse_trigger_in (controller_t const *controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->trigger_in != NULL);

    digital_pin_level_set_high(controller->trigger_in);
    time_sleep_ms(START_MOVE_PULSE_WIDTH_MS);
    digital_pin_level_set_low(controller->trigger_in);

    return;
}

controller_status_t
controller_write_analog_in (controller_t const *controller, uint16_t value)
{
    ASSERT(controller != NULL);
    ASSERT(controller->analog_in != NULL);

    if (analog_output_write(controller->analog_in, value)
        != ANALOG_OUTPUT_STATUS_OK)
    {
        return CONTROLLER_STATUS_ERR;
    }

    return CONTROLLER_STATUS_OK;
}

/** @brief Record the stopping of stage movement. */
static void
trigger_out_isr (eic_extint_line_t line, void *context)
{
    ASSERT(context != NULL);

    (void)line;

    controller_t *controller = (controller_t *)context;

    controller_set_stage_moving(controller, false);

    return;
}
