#ifndef APP_AXIS_H
#define APP_AXIS_H

#include "app/controller.h"
#include <stddef.h>
#include <stdint.h>

/** @brief Interface between coordinate system and controller. */
typedef struct
{
    /** Minimum coordinate on the axis in units. */
    int min;

    /** Maximum coordinate on the axis in units. */
    int max;

    /** Length of each unit on the axis in nanometres */
    int unit_nm;

    /** Analog reading from the where the stage was calibrated to initially. */
    uint16_t origin_analog_val;

    /** State variable for current coordinate on the axis in units. */
    int current;

    /** State variable for target coordinate on the axis in units. */
    int target;

    /** Pointer to the controller for the corresponding axis. */
    app_controller_t *controller;
} app_axis_t;

/** @brief Initialize an axis structure. */
void app_axis_init(app_axis_t       *axis,
                   int               min,
                   int               max,
                   int               unit_nm,
                   uint16_t          origin_analog_val,
                   app_controller_t *controller);

/** @brief Number of points on the axis. */
size_t app_axis_num_points(const app_axis_t *axis);

/**
 * @brief Set the target of the axis.
 *
 * It sets the target of the stage to the point corresponding to a given
 * coordinate.
 */
void app_axis_set_target(app_axis_t *axis, int target);

/** @brief Start the axis' movement to its target. */
void app_axis_start_move(app_axis_t *axis);

/** @brief Return whether the axis is moving or not */
bool app_axis_is_moving(app_axis_t *axis);

/** @brief Update state variables when the stage is at its destination. */
void app_axis_move_end(app_axis_t *axis);

#endif
