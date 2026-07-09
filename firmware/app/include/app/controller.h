/**
 * @brief Controller I/O and state layer.
 *
 * NOTE: This module will only work as expected if the controllers are in the
 *       closed-loop control mode, with both input and output analog voltages
 *       ranging from 0 to 10 volts.
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "board/indio/analog_output.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief Interface with the controller I/O and Pico interrupts. */
typedef struct
{
    /** Data on the digital pin connected to the controller Trigger IN. */
    platform_samd21g18a_pin_t const *trigger_in;

    /** Data on the analog output connected to the controller Analog IN. */
    board_indio_analog_output_channel_t const *analog_in;

    /** Interrupt pin connected to the controller Trigger OUT. */
    platform_samd21g18a_eic_pin_t const *trigger_out;

    /** State variable for whether the stage is moving. */
    volatile bool stage_moving;
} app_controller_t;

/** @brief Initialize a controller structure. */
void app_controller_init(app_controller_t                          *controller,
                         platform_samd21g18a_pin_t const           *trigger_in,
                         platform_samd21g18a_eic_pin_t const       *trigger_out,
                         board_indio_analog_output_channel_t const *analog_in);

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
app_controller_get_stage_moving (app_controller_t const *controller)
{
    return controller->stage_moving;
}

/**
 * @brief Start the stage's movement to its target by pulsing HIGH the
 *        controller Trigger IN.
 */
void app_controller_pulse_trigger_in(app_controller_t const *controller);

/** @brief Set the target of the stage through the controller Analog IN. */
void app_controller_write_analog_in(app_controller_t const *controller,
                                    uint16_t                dac_value);

#endif
