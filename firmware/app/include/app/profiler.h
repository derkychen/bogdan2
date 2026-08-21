/**
 * @file profiler.h
 * @brief Main logical entry point.
 *
 * This module contains the main logical sequence of events when profiling a
 * beam. It contains functions to profile a beam in all three supported modes.
 * It provides a structure that is initialized each time a beam is to be
 * profiled.
 */
#ifndef APP_PROFILER_H
#define APP_PROFILER_H

#include "app/controller.h"
#include "app/parameters.h"
#include "app/path.h"
#include "app/relay.h"
#include "app/serial.h"

/** @brief Profiler status codes. */
typedef enum
{
    PROFILER_STATUS_OK = 0,
    PROFILER_STATUS_ERR,
    PROFILER_STATUS_ERR_MOVE_COMMAND_TIMEOUT,
    PROFILER_STATUS_ERR_X_AXIS_INIT,
    PROFILER_STATUS_ERR_Y_AXIS_INIT,
    PROFILER_STATUS_ERR_PATH_INIT,
    PROFILER_STATUS_ERR_TARGET,
    PROFILER_STATUS_ERR_AXES_TIMEOUT,
    PROFILER_STATUS_ERR_RELAY_TIMEOUT,
} profiler_status_t;

/** @brief Task function
 *
 * This function is called in long (approximately longer than 1 millisecond),
 * blocking loops.
 */
typedef void (*profiler_task_t)(void);

/** @brief Abstraction of the beam profiler. */
typedef struct
{
    controller_t *x_controller; /**< Controller for the x-axis. */
    controller_t *y_controller; /**< Controller for the y-axis. */
    relay_t      *relay;        /**< Laser trigger relay. */

    path_raster_direction_t
        prev_raster_direction; /**< Previous path raster direction.*/

    serial_buf_t   *in;   /**< Serial reading buffer. */
    profiler_task_t task; /**< Task function to be called in blocking delays. */
} profiler_t;

/** @brief Initialize the beam profiler. */
void profiler_init(profiler_t     *profiler,
                   controller_t   *x_controller,
                   controller_t   *y_controller,
                   relay_t        *relay,
                   serial_buf_t   *in,
                   profiler_task_t task);

/** @brief Profile a beam based on parameters. */
profiler_status_t profiler_profile(profiler_t         *profiler,
                                   parameters_t const *parameters);

#endif
