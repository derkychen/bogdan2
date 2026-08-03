/**
 * @file relay.c
 * @brief Implementation of relay functionality.
 *
 * NOTE: The interrupt service routine is only required for counting, as the
 *       event system does not go through `EIC_Handler`.
 */
#include "app/relay.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/evsys.h"
#include "platform/samd21g18a/pulser.h"
#include <stddef.h>

#define PULSE_WIDTH_TICKS (4800u)

static void count_isr(eic_extint_line_t line, void *context);

void
relay_init (relay_t *relay, pulser_t const *out, eic_pin_t const *in)
{
    ASSERT(relay != NULL);
    ASSERT(in != NULL);
    ASSERT(out != NULL);

    relay->out   = out;
    relay->in    = in;
    relay->count = 0;

    pulser_configure(relay->out);

    eic_configure(relay->in, EIC_SENSE_RISE);
    eic_register_callback_entry(relay->in->line, count_isr, relay);
    eic_interrupt_disable(relay->in->line);

    evsys_channel_set_generator(EVSYS_CHANNEL_0,
                                eic_event_generator(relay->in->line),
                                EVSYS_PATH_ASYNCHRONOUS);
    evsys_user_set_channel(pulser_event_user(relay->out), EVSYS_CHANNEL_0);

    pulser_width_set(relay->out, PULSE_WIDTH_TICKS);
}

void
relay_count_start (relay_t *relay)
{
    ASSERT(relay != NULL);
    ASSERT(relay->in != NULL);

    relay->count = 0;

    eic_interrupt_enable(relay->in->line);

    return;
}

uint32_t
relay_count_get (relay_t const *relay)
{
    ASSERT(relay != NULL);

    return relay->count;
}

void
relay_count_end (relay_t *relay)
{
    ASSERT(relay != NULL);
    ASSERT(relay->in != NULL);

    relay->count = 0;

    eic_interrupt_disable(relay->in->line);

    return;
}

void
relay_pulser_event_start (relay_t *relay)
{
    ASSERT(relay != NULL);
    ASSERT(relay->in != NULL);

    eic_event_enable(relay->in->line);

    return;
}

void
relay_pulser_event_end (relay_t *relay)
{
    ASSERT(relay != NULL);
    ASSERT(relay->in != NULL);

    eic_event_disable(relay->in->line);

    return;
}

void
relay_pulser_retrigger (relay_t const *relay)
{
    ASSERT(relay != NULL);
    ASSERT(relay->out != NULL);

    pulser_retrigger(relay->out);

    return;
}

/** @brief Increment the counter when triggered. */
static void
count_isr (eic_extint_line_t line, void *context)
{
    ASSERT(context != NULL);

    (void)line;

    relay_t *relay = (relay_t *)context;

    relay->count++;

    return;
}
