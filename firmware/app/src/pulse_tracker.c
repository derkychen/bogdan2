#include "app/pulse_tracker.h"
#include "app/pulse_receiver.h"
#include "platform/samd21g18a/assert.h"
#include <stddef.h>

void
app_pulse_tracker_init (app_pulse_tracker_t     *tracker,
                        app_pulse_receiver_t    *receiver,
                        app_pulse_tracker_mode_t mode)
{
    PLATFORM_SAMD21G18A_ASSERT(tracker != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(mode < APP_PULSE_TRACKER_MODE_COUNT);

    tracker->receiver = receiver;
    tracker->mode     = mode;

    switch (tracker->mode)
    {
        case APP_PULSE_TRACKER_MODE_RELAY_AND_COUNT:
            app_pulse_receiver_configure_relay_and_count(receiver);
            break;

        case APP_PULSE_TRACKER_MODE_RELAY:
            app_pulse_receiver_configure_relay(receiver);
            break;

        case APP_PULSE_TRACKER_MODE_LAZY:
            break;

        case APP_PULSE_TRACKER_MODE_COUNT:
            __builtin_unreachable();

        default:
            PLATFORM_SAMD21G18A_ASSERT(false);
    }

    return;
}

uint32_t
app_pulse_tracker_get_count (app_pulse_tracker_t const *tracker)
{
    PLATFORM_SAMD21G18A_ASSERT(tracker != NULL);
    PLATFORM_SAMD21G18A_ASSERT(tracker->receiver != NULL);

    return app_pulse_receiver_get_count(tracker->receiver);
}

void
app_pulse_tracker_relay_pulse (app_pulse_tracker_t const *tracker)
{
    PLATFORM_SAMD21G18A_ASSERT(tracker != NULL);
    PLATFORM_SAMD21G18A_ASSERT(tracker->receiver != NULL);

    app_pulse_receiver_relay_pulse(tracker->receiver);

    return;
}

void
app_pulse_tracker_start (app_pulse_tracker_t const *tracker)
{
    PLATFORM_SAMD21G18A_ASSERT(tracker != NULL);
    PLATFORM_SAMD21G18A_ASSERT(tracker->receiver != NULL);

    if (tracker->mode == APP_PULSE_TRACKER_MODE_RELAY_AND_COUNT)
    {
        app_pulse_receiver_count_reset(tracker->receiver);
    }

    app_pulse_receiver_interrupts_enable(tracker->receiver);

    return;
}

void
app_pulse_tracker_end (app_pulse_tracker_t const *tracker)
{
    PLATFORM_SAMD21G18A_ASSERT(tracker != NULL);
    PLATFORM_SAMD21G18A_ASSERT(tracker->receiver != NULL);

    if (tracker->mode == APP_PULSE_TRACKER_MODE_RELAY_AND_COUNT)
    {
        app_pulse_receiver_count_reset(tracker->receiver);
    }

    app_pulse_receiver_interrupts_disable(tracker->receiver);

    return;
}
