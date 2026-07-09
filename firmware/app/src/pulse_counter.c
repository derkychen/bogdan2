#include "app/pulse_counter.h"
#include "platform/samd21g18a/assert.h"
#include <stddef.h>

/** @brief Increment counter upon trigger rising edge from laser/comparator. */
static void
app_pulse_counter_irq_handler (platform_samd21g18a_eic_extint_line_t line,
                               void                                 *context)
{
    app_pulse_counter_t *pulse_counter;

    PLATFORM_SAMD21G18A_ASSERT(context != NULL);

    (void)line;

    pulse_counter = (app_pulse_counter_t *)context;

    pulse_counter->count++;

    return;
}

void
app_pulse_counter_init (app_pulse_counter_t                 *pulse_counter,
                        platform_samd21g18a_eic_pin_t const *trigger)
{
    platform_samd21g18a_eic_cfg_t trigger_cfg;

    PLATFORM_SAMD21G18A_ASSERT(pulse_counter != NULL);
    PLATFORM_SAMD21G18A_ASSERT(trigger != NULL);

    pulse_counter->trigger = trigger;
    pulse_counter->count   = 0;

    trigger_cfg = (platform_samd21g18a_eic_cfg_t) {
        .eic_pin = trigger,
        .sense   = PLATFORM_SAMD21G18A_EIC_SENSE_RISE,
    };

    platform_samd21g18a_eic_configure(&trigger_cfg);

    platform_samd21g18a_eic_register_callback_entry(
        trigger_cfg.eic_pin->line,
        app_pulse_counter_irq_handler,
        pulse_counter);

    return;
}

void
app_pulse_counter_reset (app_pulse_counter_t *pulse_counter)
{
    PLATFORM_SAMD21G18A_ASSERT(pulse_counter != NULL);

    pulse_counter->count = 0;

    return;
}

uint32_t
app_pulse_counter_get (app_pulse_counter_t *pulse_counter)
{
    PLATFORM_SAMD21G18A_ASSERT(pulse_counter != NULL);

    return pulse_counter->count;
}
