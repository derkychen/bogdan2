/**
 * @file controller.h
 * @brief Controller I/O and state layer.
 *
 * NOTE: This module will only work as expected if the controllers are in the
 *       closed-loop control mode, with both output analog voltage ranging from
 *       0 to 10 volts.
 */
#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "board/indio/analog_output.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief Controller status codes. */
typedef enum
{
    CONTROLLER_STATUS_ANALOG_IN_OK = 0,
    CONTROLLER_STATUS_ANALOG_IN_ERR,
} controller_status_t;

/** @brief Interface with the controller I/O and Pico interrupts. */
typedef struct
{
    /** Data on the digital pin connected to the controller Trigger IN. */
    pin_t const *trigger_in;

    /** Data on the analog output connected to the controller Analog IN. */
    analog_output_channel_t const *analog_in;

    /** Interrupt pin connected to the controller Trigger OUT. */
    eic_pin_t const *trigger_out;

    /** State variable for whether the stage is moving. */
    volatile bool stage_moving;
} controller_t;

/** @brief Initialize and configure a controller. */
void controller_init(controller_t              *controller,
                         pin_t const                   *trigger_in,
                         eic_pin_t const               *trigger_out,
                         analog_output_channel_t const *analog_in);

/** @brief Return whether the stage is moving or not. */
bool controller_get_stage_moving(controller_t const *controller);

/**
 * @brief Set the state of the stage (i.e. moving or not).
 *
 * When called directly and not by an interrupt, this function should be setting
 * `stage_moving` to `true`.
 */
void controller_set_stage_moving(controller_t *controller,
                                     bool              stage_moving);

/** @brief Disable the controller's interrupt line. */
void controller_interrupt_disable(controller_t const *controller);

/** @brief Enable the controller's interrupt line. */
void controller_interrupt_enable(controller_t const *controller);

/**
 * @brief Start the stage's movement to its target by pulsing HIGH the
 *        controller Trigger IN.
 */
void controller_pulse_trigger_in(controller_t const *controller);

/** @brief Set the target of the stage through the controller Analog IN. */
controller_status_t controller_write_analog_in(
    controller_t const *controller, uint16_t dac_value);

#endif
