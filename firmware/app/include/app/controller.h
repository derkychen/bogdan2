/**
 * @file controller.h
 * @brief Controller I/O and state layer.
 *
 * @note This module will only work as expected if the controllers are in the
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
    CONTROLLER_STATUS_OK = 0,
    CONTROLLER_STATUS_ERR,
} controller_status_t;

/** @brief Interface with the controller I/O and Pico interrupts. */
typedef struct
{
    pin_t const                   *trigger_in; /**< Connected to Trigger In. */
    analog_output_channel_t const *analog_in;  /**< Connected to Analog In. */
    eic_pin_t const *trigger_out;              /**< Connected to Trigger Out. */

    volatile bool     stage_moving;         /**< Whether the stage is moving. */
    volatile uint16_t target_analog_value;  /**< Target Analog In value. */
    volatile uint16_t current_analog_value; /**< Current Analog In value. */
} controller_t;

/** @brief Initialize and configure a controller. */
void controller_init(controller_t                  *controller,
                     pin_t const                   *trigger_in,
                     eic_pin_t const               *trigger_out,
                     analog_output_channel_t const *analog_in);

/** @brief Whether the stage is moving or not. */
bool controller_get_stage_moving(controller_t const *controller);

/**
 * @brief Set the state of the stage (i.e. moving or not).
 *
 * When called directly and not by an interrupt, this function should be setting
 * `stage_moving` to `true`.
 */
void controller_set_stage_moving(controller_t *controller, bool stage_moving);

/**
 * @brief Whether the stage should move or not.
 *
 * That is, whether the supplied target position differs from the analog value
 * of the current position.
 */
bool controller_should_move(controller_t const *controller);

/** @brief Disable the controller's interrupt line. */
void controller_interrupt_disable(controller_t const *controller);

/** @brief Enable the controller's interrupt line. */
void controller_interrupt_enable(controller_t const *controller);

/**
 * @brief Start the stage's movement to its target by pulsing HIGH the
 *        controller Trigger In.
 */
void controller_pulse_trigger_in(controller_t const *controller);

/** @brief Set the target of the stage through the controller Analog In. */
controller_status_t controller_write_analog_in(controller_t *controller,
                                               uint16_t      dac_value);

/**
 * @brief Invalidate the current position.
 *
 * This should be called when location is uncertain, for example after a
 * movement timeout.
 */
void controller_invalidate_current(controller_t *controller);

#endif
