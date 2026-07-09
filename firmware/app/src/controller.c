#include "app/controller.h"
#include "board/indio/analog_output.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"
#include <stddef.h>

#define START_MOVE_PULSE_WIDTH_MSEC (1u)

/** @brief Record the stopping of stage movement. */
static void
app_controllers_trigger_out_irq_handler (
    platform_samd21g18a_eic_extint_line_t line, void *context)
{
    app_controller_t *controller;

    PLATFORM_SAMD21G18A_ASSERT(context != NULL);

    (void)line;
    controller = (app_controller_t *)context;

    app_controller_set_stage_moving(controller, false);

    return;
}

void
app_controller_init (app_controller_t                          *controller,
                     platform_samd21g18a_pin_t const           *trigger_in,
                     platform_samd21g18a_eic_pin_t const       *trigger_out,
                     board_indio_analog_output_channel_t const *analog_in)
{
    platform_samd21g18a_eic_cfg_t trigger_out_cfg;

    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(trigger_in != NULL);
    PLATFORM_SAMD21G18A_ASSERT(trigger_out != NULL);
    PLATFORM_SAMD21G18A_ASSERT(analog_in != NULL);

    controller->trigger_in   = trigger_in;
    controller->trigger_out  = trigger_out;
    controller->analog_in    = analog_in;
    controller->stage_moving = false;

    trigger_out_cfg = (platform_samd21g18a_eic_cfg_t) {
        .eic_pin = trigger_out,
        .sense   = PLATFORM_SAMD21G18A_EIC_SENSE_RISE,
    };

    platform_samd21g18a_eic_configure(&trigger_out_cfg);

    platform_samd21g18a_eic_register_callback_entry(
        trigger_out_cfg.eic_pin->line,
        app_controllers_trigger_out_irq_handler,
        controller);
}

void
app_controller_pulse_trigger_in (app_controller_t const *controller)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);

    platform_samd21g18a_digital_pin_level_set_high(controller->trigger_in);
    platform_samd21g18a_time_sleep_msec(START_MOVE_PULSE_WIDTH_MSEC);
    platform_samd21g18a_digital_pin_level_set_low(controller->trigger_in);
}

void
app_controller_write_analog_in (app_controller_t const *controller,
                                uint16_t                value)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);

    (void)board_indio_analog_output_write(controller->analog_in, value);
}
