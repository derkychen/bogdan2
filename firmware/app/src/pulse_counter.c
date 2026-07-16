#include "app/pulse_counter.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"
#include <stddef.h>

#define RELAY_PULSE_WIDTH_USEC (1U)

/** @brief Increment the counter upon a rising edge trigger. */
static void
pulse_counter_isr (platform_samd21g18a_eic_extint_line_t line, void *context)
{
    app_pulse_counter_t *counter;

    PLATFORM_SAMD21G18A_ASSERT(context != NULL);

    (void)line;

    counter = (app_pulse_counter_t *)context;

    counter->count++;

    // Pulse the relay pin.
    platform_samd21g18a_digital_pin_level_set_high(counter->relay);
    platform_samd21g18a_time_sleep_usec(RELAY_PULSE_WIDTH_USEC);
    platform_samd21g18a_digital_pin_level_set_low(counter->relay);

    return;
}

void
app_pulse_counter_init (app_pulse_counter_t                     *counter,
                        platform_samd21g18a_eic_pin_t const     *trigger,
                        platform_samd21g18a_digital_pin_t const *relay)
{
    platform_samd21g18a_eic_cfg_t trigger_cfg;

    PLATFORM_SAMD21G18A_ASSERT(counter != NULL);
    PLATFORM_SAMD21G18A_ASSERT(trigger != NULL);
    PLATFORM_SAMD21G18A_ASSERT(relay != NULL);

    counter->trigger = trigger;
    counter->relay   = relay;
    counter->count   = 0;

    trigger_cfg = (platform_samd21g18a_eic_cfg_t) {
        .eic_pin = trigger,
        .sense   = PLATFORM_SAMD21G18A_EIC_SENSE_RISE,
    };

    platform_samd21g18a_eic_configure(&trigger_cfg);

    platform_samd21g18a_eic_register_callback_entry(
        trigger_cfg.eic_pin->line, pulse_counter_isr, counter);

    platform_samd21g18a_eic_line_disable(counter->trigger->line);

    platform_samd21g18a_digital_pin_cfg_set_output(counter->relay);
    platform_samd21g18a_digital_pin_level_set_low(counter->relay);

    return;
}

uint32_t
app_pulse_counter_get_count (app_pulse_counter_t *counter)
{
    PLATFORM_SAMD21G18A_ASSERT(counter != NULL);

    return counter->count;
}

void
app_pulse_counter_start (app_pulse_counter_t *counter)
{
    PLATFORM_SAMD21G18A_ASSERT(counter != NULL);
    PLATFORM_SAMD21G18A_ASSERT(counter->trigger != NULL);

    counter->count = 0;

    platform_samd21g18a_eic_line_enable(counter->trigger->line);

    return;
}

void
app_pulse_counter_end (app_pulse_counter_t *counter)
{
    PLATFORM_SAMD21G18A_ASSERT(counter != NULL);
    PLATFORM_SAMD21G18A_ASSERT(counter->trigger != NULL);

    counter->count = 0;

    platform_samd21g18a_eic_line_disable(counter->trigger->line);

    return;
}
