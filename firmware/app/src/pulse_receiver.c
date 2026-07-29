#include "app/pulse_receiver.h"
#include "platform/samd21g18a/pulse_generator.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/eic.h"
#include <stddef.h>

// NOTE: This renders it impossible to profile lasers that trigger more
//       frequently than 10 kilohertz.
#define RELAY_PULSE_WIDTH_TICKS (4800u)

static void count_isr(platform_samd21g18a_eic_extint_line_t line,
                      void                                 *context);

void
app_pulse_receiver_init (app_pulse_receiver_t                        *receiver,
                         platform_samd21g18a_eic_pin_t const         *trigger,
                         platform_samd21g18a_pulse_generator_t const *relay)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(trigger != NULL);
    PLATFORM_SAMD21G18A_ASSERT(relay != NULL);

    receiver->trigger = trigger;
    receiver->relay   = relay;
    receiver->count   = 0;

    return;
}

void
app_pulse_receiver_configure_relay (app_pulse_receiver_t const *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->relay != NULL);

    receiver->relay->init();
    receiver->relay->event_disable(receiver->trigger->line);
    receiver->relay->width_set(RELAY_PULSE_WIDTH_TICKS);

    return;
}

void
app_pulse_receiver_configure_count (app_pulse_receiver_t *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->trigger != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->relay != NULL);

    platform_samd21g18a_eic_cfg_t trigger_cfg
        = (platform_samd21g18a_eic_cfg_t) {
              .eic_pin = receiver->trigger,
              .sense   = PLATFORM_SAMD21G18A_EIC_SENSE_RISE,
          };

    platform_samd21g18a_eic_configure(&trigger_cfg);

    platform_samd21g18a_eic_register_callback_entry(
        receiver->trigger->line, count_isr, receiver);

    platform_samd21g18a_eic_line_disable(receiver->trigger->line);

    return;
}

void
app_pulse_receiver_event_disable (app_pulse_receiver_t const *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->trigger != NULL);

    receiver->relay->event_disable(receiver->trigger->line);

    return;
}

void
app_pulse_receiver_event_enable (app_pulse_receiver_t const *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->trigger != NULL);

    receiver->relay->event_enable(receiver->trigger->line);

    return;
}

void
app_pulse_receiver_interrupt_disable (app_pulse_receiver_t const *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->trigger != NULL);

    platform_samd21g18a_eic_line_disable(receiver->trigger->line);

    return;
}

void
app_pulse_receiver_interrupt_enable (app_pulse_receiver_t const *receiver)
{
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver->trigger != NULL);

    platform_samd21g18a_eic_line_enable(receiver->trigger->line);

    return;
}

uint32_t
app_pulse_receiver_get_count (app_pulse_receiver_t const *receiver)
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
    PLATFORM_SAMD21G18A_ASSERT(receiver->relay != NULL);

    receiver->relay->retrigger();
}

/** @brief Increment the counter when triggered when counting is enabled. */
static void
count_isr (platform_samd21g18a_eic_extint_line_t line, void *context)
{
    PLATFORM_SAMD21G18A_ASSERT(context != NULL);

    (void)line;

    app_pulse_receiver_t *receiver = (app_pulse_receiver_t *)context;

    receiver->count++;

    return;
}
