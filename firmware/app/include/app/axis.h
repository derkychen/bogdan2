#ifndef APP_AXIS_H
#define APP_AXIS_H

#include "app/controller.h"
#include <stddef.h>
#include <stdint.h>

/** @brief Axis status codes. */
typedef enum
{
    APP_AXIS_STATUS_INIT_OK = 0,
    APP_AXIS_STATUS_TARGET_OK,
    APP_AXIS_STATUS_INIT_ERR_MIN_TOO_LOW,
    APP_AXIS_STATUS_INIT_ERR_MIN_GREATER_THAN_MAX,
    APP_AXIS_STATUS_INIT_ERR_MAX_TOO_HIGH,
    APP_AXIS_STATUS_INIT_ERR_UNIT_SMALLER_THAN_TOLERANCE,
    APP_AXIS_STATUS_INIT_ERR_UNIT_TOO_LARGE,
    APP_AXIS_STATUS_INIT_ERR_ORIGIN_OUTSIDE_RANGE,
    APP_AXIS_STATUS_INIT_ERR_BOUNDS_OUTSIDE_RANGE,
    APP_AXIS_STATUS_TARGET_ERR_CONTROLLER,
} app_axis_status_t;

/** @brief Interface between coordinate system and controller. */
typedef struct
{
    /** Minimum coordinate on the axis in units. */
    int min;

    /** Maximum coordinate on the axis in units. */
    int max;

    /** Length of each unit on the axis in nanometres */
    uint32_t unit_nm;

    /** Position in nanometres the stage was calibrated to initially. */
    int origin_nm;

    /** State variable for current coordinate on the axis in units. */
    int current;

    /** State variable for target coordinate on the axis in units. */
    int target;

    /** Pointer to the controller for the corresponding axis. */
    app_controller_t *controller;
} app_axis_t;

/** @brief Initialize an axis structure. */
app_axis_status_t app_axis_init(app_axis_t       *axis,
                                int               min,
                                int               max,
                                uint32_t          unit_nm,
                                int               origin_nm,
                                app_controller_t *controller);

/** @brief Return whether the axis is moving or not */
bool app_axis_get_stage_moving(app_axis_t const *axis);

/** @brief Number of points on the axis. */
size_t app_axis_num_points(app_axis_t const *axis);

/**
 * @brief Set the target of the axis.
 *
 * It sets the target of the stage to the point corresponding to a given
 * coordinate.
 */
app_axis_status_t app_axis_set_target(app_axis_t *axis, int target);

/** @brief Start the axis' movement to its target. */
void app_axis_move_start(app_axis_t *axis);

/** @brief Update state variables when the stage is at its destination. */
void app_axis_move_end(app_axis_t *axis);

#endif
