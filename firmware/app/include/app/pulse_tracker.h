#ifndef APP_PULSE_TRACKER_H
#define APP_PULSE_TRACKER_H

#include "app/pulse_receiver.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief Enumeration of pulse tracking modes. */
typedef enum
{
    APP_PULSE_TRACKER_MODE_RELAY_AND_COUNT = 0,
    APP_PULSE_TRACKER_MODE_RELAY,
    APP_PULSE_TRACKER_MODE_LAZY,

    APP_PULSE_TRACKER_MODE_COUNT,
} app_pulse_tracker_mode_t;

/** @brief Laser pulse counter. */
typedef struct
{
    /** Receiver on which pulse tracking logic is built. */
    app_pulse_receiver_t *receiver;

    /** Pulse tracker mode. */
    app_pulse_tracker_mode_t mode;
} app_pulse_tracker_t;

/** @brief Initialize pulse tracker. */
void app_pulse_tracker_init(app_pulse_tracker_t     *tracker,
                            app_pulse_receiver_t          *receiver,
                            app_pulse_tracker_mode_t mode);

/** @brief Return the pulse count. */
uint32_t app_pulse_tracker_get_count(app_pulse_tracker_t const *tracker);

/** @brief Pulse the relay pin. */
void app_pulse_tracker_relay_pulse(app_pulse_tracker_t const *tracker);

/** @brief Enable interrupts from the receiver trigger. */
void app_pulse_tracker_start(app_pulse_tracker_t const *tracker);

/** @brief Disable interrupts from the receiver trigger. */
void app_pulse_tracker_end(app_pulse_tracker_t const *tracker);

#endif
