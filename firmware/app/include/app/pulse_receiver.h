#ifndef APP_PULSE_RECEIVER_H
#define APP_PULSE_RECEIVER_H

#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/pulse_generator.h"
#include <stdint.h>

/** @brief Laser pulse receiver. */
typedef struct
{
    /** EIC pin connected to the comparator or laser trigger. */
    platform_samd21g18a_eic_pin_t const *trigger;

    /** Digital pin used to relay the pulse signal. */
    platform_samd21g18a_pulse_generator_t const *relay;

    /** The number of pulses detected after the last reset. */
    volatile uint32_t count;
} app_pulse_receiver_t;

/** @brief Initialize pulse counter. */
void app_pulse_receiver_init(
    app_pulse_receiver_t                        *receiver,
    platform_samd21g18a_eic_pin_t const         *trigger,
    platform_samd21g18a_pulse_generator_t const *relay);

/** @brief Configure the one-shot relay pulse generator. */
void app_pulse_receiver_configure_relay(app_pulse_receiver_t const *receiver);

/** @brief Configure the counting interrupt. */
void app_pulse_receiver_configure_count(app_pulse_receiver_t *receiver);

/** @brief Disable the receiver's event detection. */
void app_pulse_receiver_event_disable(app_pulse_receiver_t const *receiver);

/** @brief Enable the receiver's event detection. */
void app_pulse_receiver_event_enable(app_pulse_receiver_t const *receiver);

/** @brief Disable the receiver's interrupt detection. */
void app_pulse_receiver_interrupt_disable(app_pulse_receiver_t const *receiver);

/** @brief Enable the receiver's interrupt detection. */
void app_pulse_receiver_interrupt_enable(app_pulse_receiver_t const *receiver);

/** @brief Get the current pulse count. */
uint32_t app_pulse_receiver_get_count(app_pulse_receiver_t const *receiver);

/** @brief Reset the current pulse count. */
void app_pulse_receiver_count_reset(app_pulse_receiver_t *receiver);

/** @brief Pulse the relay pin. */
void app_pulse_receiver_relay_pulse(app_pulse_receiver_t const *receiver);

#endif
