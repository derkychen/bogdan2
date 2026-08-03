/**
 * @file axis.h
 * @brief Module that maps coordinate system to controllers.
 *
 * Each axis serves as an interface between application behaviour and controller
 * I/O. For example, starting stage movement maps to pulsing Trigger IN, and
 * moving to a certain coordinate on an axis results in the writing of a
 * calculated analog voltage to Analog IN.
 */
#ifndef APP_AXIS_H
#define APP_AXIS_H

#include "app/controller.h"
#include <stddef.h>
#include <stdint.h>

/** @brief Axis status codes. */
typedef enum
{
    AXIS_STATUS_INIT_OK = 0,
    AXIS_STATUS_TARGET_OK,
    AXIS_STATUS_INIT_ERR_MIN_TOO_SMALL,
    AXIS_STATUS_INIT_ERR_MIN_GREATER_THAN_MAX,
    AXIS_STATUS_INIT_ERR_MAX_TOO_LARGE,
    AXIS_STATUS_INIT_ERR_UNIT_SMALLER_THAN_TOLERANCE,
    AXIS_STATUS_INIT_ERR_UNIT_TOO_LARGE,
    AXIS_STATUS_INIT_ERR_ORIGIN_OUTSIDE_RANGE,
    AXIS_STATUS_INIT_ERR_BOUNDS_OUTSIDE_RANGE,
    AXIS_STATUS_TARGET_ERR_CONTROLLER,
} axis_status_t;

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

    /** Pointer to the controller for the corresponding axis. */
    controller_t *controller;
} axis_t;

/** @brief Initialize an axis structure and configure controller interrupts. */
axis_status_t axis_init(axis_t       *axis,
                        int           min,
                        int           max,
                        uint32_t      unit_nm,
                        int           origin_nm,
                        controller_t *controller);

/** @brief Return whether the axis is moving or not */
bool axis_get_stage_moving(axis_t const *axis);

/** @brief Number of points on the axis. */
size_t axis_num_points(axis_t const *axis);

/**
 * @brief Set the target of the axis.
 *
 * It sets the target of the stage to the point corresponding to a given
 * coordinate.
 */
axis_status_t axis_set_target(axis_t *axis, int target);

/** @brief Start the axis' movement to its target. */
void axis_move_start(axis_t *axis);

/** @brief Update state variables when the stage is at its destination. */
void axis_move_end(axis_t *axis);

#endif
