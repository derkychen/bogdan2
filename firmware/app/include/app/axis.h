/**
 * @file axis.h
 * @brief Maps coordinate system to controllers.
 *
 * Each axis serves as an interface between application behaviour and controller
 * I/O. For example, starting stage movement maps to pulsing Trigger IN, and
 * moving to a certain coordinate on an axis results in the writing of a
 * calculated analog voltage to Analog IN.
 */
#ifndef APP_AXIS_H
#define APP_AXIS_H

#include "app/controller.h"
#include <stdint.h>

/** @brief Axis status codes. */
typedef enum
{
    AXIS_STATUS_OK = 0,
    AXIS_STATUS_ERR_MIN_TOO_SMALL,
    AXIS_STATUS_ERR_MIN_GREATER_THAN_MAX,
    AXIS_STATUS_ERR_MAX_TOO_LARGE,
    AXIS_STATUS_ERR_UNIT_SMALLER_THAN_MIN_STEP,
    AXIS_STATUS_ERR_UNIT_TOO_LARGE,
    AXIS_STATUS_ERR_ORIGIN_OUTSIDE_RANGE,
    AXIS_STATUS_ERR_BOUNDS_OUTSIDE_RANGE,
    AXIS_STATUS_ERR_CONTROLLER,
} axis_status_t;

/** @brief Interface between coordinate system and controller. */
typedef struct
{
    int           min;        /**< Minimum coordinate in units. */
    int           max;        /**< Maximum coordinate in units. */
    uint32_t      unit_nm;    /**< Length of each unit in nanometres. */
    int           origin_nm;  /**< Initial stage position in nanometres. */
    controller_t *controller; /**< Pointer to the controller for the axis. */
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
