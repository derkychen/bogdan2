/**
 * @brief Controller I/O control.
 *
 * NOTE: This module will only work as expected if the controllers are in the
 * closed-loop control mode.
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <pico/types.h>

/** @brief Interface with the controller I/O and Pico interrupts. */
typedef struct Controller
{
    /** The digital pin that the controller Trigger IN is connected to. */
    uint trigger_in;

    /** The analog pin that the controller Analog IN is connected to. */
    uint analog_in;

    /** The digital pin that the controller Trigger OUT is connected to. */
    uint trigger_out;

    /**
     * The ADC channel that the controller Analog OUT is connected to.
     *
     * NOTE: This value differs from the actual GPIO pin number. For example,
     * GPIO pin 26 corresponds to ADC channel 0.
     */
    uint analog_out_channel;

    /** State variable for whether the stage is moving. */
    volatile bool stage_moving;
} Controller;

/** @brief Initialize a Controller structure. */
void controller_init(Controller *controller,
                     uint        trigger_in,
                     uint        analog_in,
                     uint        trigger_out,
                     uint        analog_out_channel);

/**
 * @brief Initialize IRQ configurations for both stages.
 *
 * This allows both controller Trigger OUTs to send interrupts to indicate the
 * stages reaching their targets.
 */
void controllers_irq_init(void);

/** @brief Set the state of the stage to be moving. */
void controller_set_stage_moving(Controller *controller);

/**
 * @brief Start the stage's movement to its target by pulsing HIGH the
 *        controller Trigger IN.
 */
void controller_pulse_trigger_in(const Controller *controller);

/** @brief Set the target of the stage through the controller Analog IN. */
void controller_write_analog_in(const Controller *controller,
                                uint16_t          analog_value);

/**
 * @brief Read the value corresponding to the stage's position from the
 *        controller Analog OUT.
 */
uint16_t controller_read_analog_out(const Controller *controller);

/** @brief Return whether the stage is moving or not. */
bool controller_is_stage_moving(const Controller *controller);

#endif
