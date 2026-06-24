#include "app/controller.h"
#include "platform/samd21g18a/adc.h"
#include "platform/samd21g18a/dac.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"

#define START_MOVE_PULSE_WIDTH_MSEC (1u)

/** @brief Record the stopping of stage movement. */
static void
app_controllers_trigger_out_irq_handler (uint8_t extint_line, void *context)
{
    app_controller_t *controller;

    (void)extint_line;
    controller = (app_controller_t *)context;

    app_controller_set_stage_moving(controller, false);

    return;
}

void
app_controller_init (app_controller_t                        *controller,
                     const platform_samd21g18a_pin_digital_t *trigger_in,
                     const platform_samd21g18a_pin_dac_t     *analog_in,
                     const platform_samd21g18a_pin_eic_t     *trigger_out,
                     const platform_samd21g18a_pin_adc_t     *analog_out)
{
    controller->trigger_in   = trigger_in;
    controller->analog_in    = analog_in;
    controller->trigger_out  = trigger_out;
    controller->analog_out   = analog_out;
    controller->stage_moving = false;

    platform_samd21g18a_eic_config_pin_sense(
        trigger_out, PLATFORM_SAMD21G18A_EIC_SENSE_RISE);

    platform_samd21g18a_eic_register_callback(
        trigger_out->extint_line,
        app_controllers_trigger_out_irq_handler,
        controller);
}

void
app_controller_pulse_trigger_in (const app_controller_t *controller)
{
    platform_samd21g18a_digital_pin_write_high(controller->trigger_in);
    platform_samd21g18a_time_sleep_msec(START_MOVE_PULSE_WIDTH_MSEC);
    platform_samd21g18a_digital_pin_write_low(controller->trigger_in);
}

void
app_controller_write_analog_in (const app_controller_t *controller,
                                uint16_t                dac_value)
{
    platform_samd21g18a_dac_pin_write(controller->analog_in, dac_value);
}

uint16_t
app_controller_read_analog_out (const app_controller_t *controller)
{
    return platform_samd21g18a_adc_pin_read(controller->analog_out);
}
