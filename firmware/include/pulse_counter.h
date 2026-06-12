#ifndef PULSE_COUNTER_H
#define PULSE_COUNTER_H

#include "pico/types.h"

/** @brief Laser pulse counter. */
typedef struct PulseCounter
{
    /** The digital pin that the comparator or laser trigger is connected to. */
    uint trigger;

    /** The number of pulses detected after the last reset. */
    volatile int count;
} PulseCounter;

/** @brief Status codes for initialization. */
typedef enum PulseCounterInitStatusCode
{
    PULSE_COUNTER_INIT_OK = 0,
    PULSE_COUNTER_ERR_UNSUPPORTED_GPIO,
} PulseCounterInitStatusCode;

/** @brief Initialize an PulseCounter structure. */
PulseCounterInitStatusCode pulse_counter_init(PulseCounter *pulse_counter,
                                              uint          trigger);

/** @brief Initialize IRQ configurations for the pulse counter. */
void pulse_counter_irq_init(void);

/** @brief Reset the count. */
void pulse_counter_reset(PulseCounter *pulse_counter);

/** @brief Return the count. */
int pulse_counter_get(PulseCounter *pulse_counter);

#endif
