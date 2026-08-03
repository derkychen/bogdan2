#ifndef APP_PROFILER_H
#define APP_PROFILER_H

#include "app/controller.h"
#include "app/parameters.h"
#include "app/path.h"
#include "app/relay.h"

/** @brief Profiler status codes. */
typedef enum
{
    PROFILER_STATUS_OK = 0,
    PROFILER_STATUS_ERR,
    PROFILER_STATUS_ERR_X_AXIS_INIT,
    PROFILER_STATUS_ERR_Y_AXIS_INIT,
    PROFILER_STATUS_ERR_PATH_NOT_GENERATED,
    PROFILER_STATUS_ERR_TARGET,
    PROFILER_STATUS_ERR_AXES_TIMEOUT,
    PROFILER_STATUS_ERR_PULSE_COUNTER_TIMEOUT,
} profiler_status_t;

/** @brief Task function
 *
 * This function is called in long (approximately longer than 1 millisecond)
 * blocking loops.
 */
typedef void (*profiler_task_t)(void);

/** @brief Abstraction of the beam profiler. */
typedef struct
{
    /** Controller for the x-axis. */
    controller_t *x_controller;

    /** Controller for the y-axis. */
    controller_t *y_controller;

    /** Relay. */
    relay_t *relay;

    /** Task to be called in all blocking delays.*/
    profiler_task_t task;

    /** Previous raster direction. */
    path_raster_direction_t prev_raster_direction;
} profiler_t;

/** @brief Initialize the beam profiler. */
void profiler_init(profiler_t     *profiler,
                   controller_t   *x_controller,
                   controller_t   *y_controller,
                   relay_t        *relay,
                   profiler_task_t task);

/** @brief Profile a beam based on parameters. */
profiler_status_t profiler_profile(profiler_t         *profiler,
                                   parameters_t const *parameters);

#endif
