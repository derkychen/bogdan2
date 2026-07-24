#include "app/controller.h"
#include "board/indio/analog_output.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"
#include <stddef.h>

#define START_MOVE_PULSE_WIDTH_MSEC (1u)

static void trigger_out_isr(platform_samd21g18a_eic_extint_line_t line,
                            void                                 *context);

void
app_controller_init (app_controller_t                          *controller,
                     platform_samd21g18a_pin_t const           *trigger_in,
                     platform_samd21g18a_eic_pin_t const       *trigger_out,
                     board_indio_analog_output_channel_t const *analog_in)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(trigger_in != NULL);
    PLATFORM_SAMD21G18A_ASSERT(trigger_out != NULL);
    PLATFORM_SAMD21G18A_ASSERT(analog_in != NULL);

    controller->trigger_in   = trigger_in;
    controller->trigger_out  = trigger_out;
    controller->analog_in    = analog_in;
    controller->stage_moving = false;

    platform_samd21g18a_digital_pin_cfg_set_output(trigger_in);
    platform_samd21g18a_digital_pin_level_set_low(trigger_in);

    board_indio_analog_output_write(analog_in, 0u);

    return;
}

void
app_controller_configure_trigger_out (app_controller_t *controller)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(controller->trigger_out != NULL);

    platform_samd21g18a_eic_cfg_t trigger_out_cfg
        = (platform_samd21g18a_eic_cfg_t) {
              .eic_pin = controller->trigger_out,
              .sense   = PLATFORM_SAMD21G18A_EIC_SENSE_RISE,
          };

    platform_samd21g18a_eic_configure(&trigger_out_cfg);

    platform_samd21g18a_eic_register_callback_entry(
        trigger_out_cfg.eic_pin->line, trigger_out_isr, controller);

    platform_samd21g18a_eic_line_disable(controller->trigger_out->line);

    return;
}

bool
app_controller_get_stage_moving (app_controller_t const *controller)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);

    return controller->stage_moving;
}

void
app_controller_set_stage_moving (app_controller_t *controller,
                                 bool              stage_moving)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);

    // NOTE: This value will be changed to `false` upon an interrupt.
    controller->stage_moving = stage_moving;

    return;
}

void
app_controller_interrupts_disable (app_controller_t const *controller)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(controller->trigger_out != NULL);

    platform_samd21g18a_eic_line_disable(controller->trigger_out->line);

    return;
}

void
app_controller_interrupts_enable (app_controller_t const *controller)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(controller->trigger_out != NULL);

    platform_samd21g18a_eic_line_enable(controller->trigger_out->line);

    return;
}

void
app_controller_pulse_trigger_in (app_controller_t const *controller)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(controller->trigger_in != NULL);

    platform_samd21g18a_digital_pin_level_set_high(controller->trigger_in);
    platform_samd21g18a_time_sleep_msec(START_MOVE_PULSE_WIDTH_MSEC);
    platform_samd21g18a_digital_pin_level_set_low(controller->trigger_in);

    return;
}

app_controller_status_t
app_controller_write_analog_in (app_controller_t const *controller,
                                uint16_t                value)
{
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(controller->analog_in != NULL);

    if (board_indio_analog_output_write(controller->analog_in, value)
        != BOARD_INDIO_ANALOG_OUTPUT_STATUS_OK)
    {
        return APP_CONTROLLER_STATUS_ANALOG_IN_ERR;
    }

    return APP_CONTROLLER_STATUS_ANALOG_IN_OK;
}

/** @brief Record the stopping of stage movement. */
static void
trigger_out_isr (platform_samd21g18a_eic_extint_line_t line, void *context)
{
    PLATFORM_SAMD21G18A_ASSERT(context != NULL);

    (void)line;

    app_controller_t *controller = (app_controller_t *)context;

    app_controller_set_stage_moving(controller, false);

    return;
}
