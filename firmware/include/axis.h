/**
 * @brief Coordinate system.
 *
 * Axis definition, state variables, and wrappers for controller functions.
 */
#ifndef AXIS_H
#define AXIS_H

#include <pico/types.h>
#include "controller.h"

/** @brief Interface between coordinate system and controller. */
typedef struct Axis
{
    /** Minimum coordinate on the axis in units. */
    int min;

    /** Maximum coordinate on the axis in units. */
    int max;

    /** Length of each unit on the axis in nanometres */
    int unit_nm;

    /** ADC reading from the where the stage was calibrated to initially. */
    uint16_t origin_adc_val;

    /** State variable for current coordinate on the axis in units. */
    int current;

    /** State variable for target coordinate on the axis in units. */
    int target;

    /** Pointer to the controller for the corresponding axis. */
    Controller *controller;
} Axis;

/** @brief Initialize an Axis structure. */
void axis_init(Axis       *axis,
               int         min,
               int         max,
               int         unit_nm,
               uint16_t    origin_adc_val,
               Controller *controller);

/** @brief Number of points on the axis. */
size_t axis_num_points(const Axis *axis);

/**
 * @brief Set the target of the axis.
 *
 * It sets the target of the stage to the point corresponding to a given
 * coordinate.
 */
void axis_set_target(Axis *axis, int target);

/** @brief Start the axis' movement to its target. */
void axis_start_move(Axis *axis);

/** @brief Return whether the axis is moving or not */
bool axis_is_moving(Axis *axis);

/** @brief Update state variables when the stage is at its destination. */
void axis_move_end(Axis *axis);

#endif
