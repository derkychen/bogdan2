/**
 * @brief Controller I/O and state layer.
 *
 * NOTE: This module will only work as expected if the controllers are in the
 * closed-loop control mode.
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief Interface with the controller I/O and Pico interrupts. */
typedef struct
{
    /** The digital pin that the controller Trigger IN is connected to. */
    const platform_samd21g18a_pin_digital_t *trigger_in;

    /** The DAC pin that the controller Analog IN is connected to. */
    const platform_samd21g18a_pin_dac_t *analog_in;

    /** The digital pin that the controller Trigger OUT is connected to. */
    const platform_samd21g18a_pin_eic_t *trigger_out;

    /** The ADC pin that the controller Analog OUT is connected to. */
    const platform_samd21g18a_pin_adc_t *analog_out;

    /** State variable for whether the stage is moving. */
    volatile bool stage_moving;
} app_controller_t;

/** @brief Initialize a controller structure. */
void app_controller_init(app_controller_t                        *controller,
                         const platform_samd21g18a_pin_digital_t *trigger_in,
                         const platform_samd21g18a_pin_dac_t     *analog_in,
                         const platform_samd21g18a_pin_eic_t     *trigger_out,
                         const platform_samd21g18a_pin_adc_t     *analog_out);

/**
 * @brief Set the state of the stage (i.e. moving or not).
 *
 * When called directly and not by an interrupt, this function should be setting
 * `stage_moving` to `true`.
 */
static inline void
app_controller_set_stage_moving (app_controller_t *controller,
                                 bool              stage_moving)
{
    // NOTE: This value will be changed to `false` upon an interrupt.
    controller->stage_moving = stage_moving;
}

/** @brief Return whether the stage is moving or not. */
static inline bool
app_controller_get_stage_moving (const app_controller_t *controller)
{
    return controller->stage_moving;
}

/**
 * @brief Start the stage's movement to its target by pulsing HIGH the
 *        controller Trigger IN.
 */
void app_controller_pulse_trigger_in(const app_controller_t *controller);

/** @brief Set the target of the stage through the controller Analog IN. */
void app_controller_write_analog_in(const app_controller_t *controller,
                                    uint16_t                dac_value);

/**
 * @brief Read the value corresponding to the stage's position from the
 *        controller Analog OUT.
 */
uint16_t app_controller_read_analog_out(const app_controller_t *controller);

#endif
