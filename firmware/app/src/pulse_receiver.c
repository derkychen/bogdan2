#include "app/pulse_receiver.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include <stddef.h>

#define RELAY_PULSE_WIDTH_CYCLES (40U)

/** @brief Increment the counter when triggered when counting is enabled. */
static void
relay_and_count_isr (platform_samd21g18a_eic_extint_line_t line, void *context)
{
    app_pulse_receiver_t *receiver;

    PLATFORM_SAMD21G18A_ASSERT(context != NULL);

    (void)line;

    receiver = (app_pulse_receiver_t *)context;

    app_pulse_receiver_relay_pulse(receiver);

    receiver->count++;

    return;
}

/** @brief Increment the counter when triggered when counting is enabled. */
static void
relay_isr (platform_samd21g18a_eic_extint_line_t line, void *context)
{
    app_pulse_receiver_t *receiver;

    PLATFORM_SAMD21G18A_ASSERT(context != NULL);

    (void)line;

    receiver = (app_pulse_receiver_t *)context;

    app_pulse_receiver_relay_pulse(receiver);

    return;
}

void
app_pulse_receiver_init (app_pulse_receiver_t                    *receiver,
                         platform_samd21g18a_eic_pin_t const     *trigger,
                         platform_samd21g18a_digital_pin_t const *relay)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(trigger != NULL);
    PLATFORM_SAMD21G18A_ASSERT(relay != NULL);

    receiver->trigger = trigger;
    receiver->relay   = relay;
    receiver->count   = 0;

    platform_samd21g18a_digital_pin_cfg_set_output(receiver->relay);
    platform_samd21g18a_digital_pin_level_set_low(receiver->relay);

    return;
}

void
app_pulse_receiver_configure_relay_and_count (app_pulse_receiver_t *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->trigger != NULL);

    platform_samd21g18a_eic_cfg_t trigger_cfg;

    trigger_cfg = (platform_samd21g18a_eic_cfg_t) {
        .eic_pin = receiver->trigger,
        .sense   = PLATFORM_SAMD21G18A_EIC_SENSE_RISE,
    };

    platform_samd21g18a_eic_configure(&trigger_cfg);

    platform_samd21g18a_eic_register_callback_entry(
        trigger_cfg.eic_pin->line, relay_and_count_isr, receiver);

    platform_samd21g18a_eic_line_disable(receiver->trigger->line);
}

void
app_pulse_receiver_configure_relay (app_pulse_receiver_t *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->trigger != NULL);

    platform_samd21g18a_eic_cfg_t trigger_cfg;

    trigger_cfg = (platform_samd21g18a_eic_cfg_t) {
        .eic_pin = receiver->trigger,
        .sense   = PLATFORM_SAMD21G18A_EIC_SENSE_RISE,
    };

    platform_samd21g18a_eic_configure(&trigger_cfg);

    platform_samd21g18a_eic_register_callback_entry(
        trigger_cfg.eic_pin->line, relay_isr, receiver);

    platform_samd21g18a_eic_line_disable(receiver->trigger->line);
}

void
app_pulse_receiver_interrupts_disable (app_pulse_receiver_t const *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->trigger != NULL);

    platform_samd21g18a_eic_line_disable(receiver->trigger->line);

    return;
}

void
app_pulse_receiver_interrupts_enable (app_pulse_receiver_t const *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->trigger != NULL);

    platform_samd21g18a_eic_line_enable(receiver->trigger->line);

    return;
}

uint32_t
app_pulse_receiver_get_count (app_pulse_receiver_t *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);

    return receiver->count;
}

void
app_pulse_receiver_count_reset (app_pulse_receiver_t *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);

    receiver->count = 0;

    return;
}

void
app_pulse_receiver_relay_pulse (app_pulse_receiver_t const *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);

    platform_samd21g18a_digital_pin_level_set_high(receiver->relay);

    // NOTE: The `time` module is not used here, as this delay is too short for
    //       the `sleep_usec` function to be precise.
    for (volatile uint32_t i = 0U; i < RELAY_PULSE_WIDTH_CYCLES; i++)
    {
        __NOP();
    }

    platform_samd21g18a_digital_pin_level_set_low(receiver->relay);
}
