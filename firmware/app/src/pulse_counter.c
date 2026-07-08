#include "app/pulse_counter.h"

/** @brief Increment counter upon trigger rising edge from laser/comparator. */
static void
app_pulse_counter_irq_handler (platform_samd21g18a_eic_extint_line_t line,
                               void                                 *context)
{
    app_pulse_counter_t *pulse_counter;

    (void)line;

    pulse_counter = (app_pulse_counter_t *)context;

    pulse_counter->count++;

    return;
}

void
app_pulse_counter_init (app_pulse_counter_t                 *pulse_counter,
                        platform_samd21g18a_eic_cfg_t const *trigger_cfg)
{
    pulse_counter->trigger_cfg = trigger_cfg;
    pulse_counter->count       = 0;

    platform_samd21g18a_eic_configure(trigger_cfg);

    platform_samd21g18a_eic_register_callback_entry(
        trigger_cfg->eic_pin->line,
        app_pulse_counter_irq_handler,
        pulse_counter);

    return;
}

void
app_pulse_counter_reset (app_pulse_counter_t *pulse_counter)
{
    pulse_counter->count = 0;

    return;
}

uint32_t
app_pulse_counter_get (app_pulse_counter_t *pulse_counter)
{
    return pulse_counter->count;
}
