#ifndef APP_PROFILER_H
#define APP_PROFILER_H

#include "app/controller.h"
#include "app/instruction.h"
#include "app/path.h"
#include "app/pulse_receiver.h"

/** @brief Profiler status codes. */
typedef enum
{
    APP_PROFILER_STATUS_OK = 0,
    APP_PROFILER_STATUS_ERR,
    APP_PROFILER_STATUS_ERR_X_AXIS_INIT,
    APP_PROFILER_STATUS_ERR_Y_AXIS_INIT,
    APP_PROFILER_STATUS_ERR_PATH_NOT_GENERATED,
    APP_PROFILER_STATUS_ERR_TARGET,
    APP_PROFILER_STATUS_ERR_AXES_TIMEOUT,
    APP_PROFILER_STATUS_ERR_PULSE_COUNTER_TIMEOUT,
} app_profiler_status_t;

/** @brief Task function
 *
 * This function should be called in long (> 1 millisecond) blocking loops.
 */
typedef void (*app_profiler_task_t)(void);

/** @brief Abstraction of the beam profiler. */
typedef struct
{
    /** Controller for the x-axis. */
    app_controller_t *x_controller;

    /** Controller for the y-axis. */
    app_controller_t *y_controller;

    /** Receiver. */
    app_pulse_receiver_t *receiver;

    /** Task to be called in all blocking delays.*/
    app_profiler_task_t task;

    /** Previous raster direction. */
    app_path_raster_direction_t prev_raster_direction;
} app_profiler_t;

/** @brief Initialize the beam profiler. */
app_profiler_status_t app_profiler_init(app_profiler_t       *profiler,
                                        app_controller_t     *x_controller,
                                        app_controller_t     *y_controller,
                                        app_pulse_receiver_t *receiver,
                                        app_profiler_task_t   task);

/** @brief Profile a beam based on instructions. */
app_profiler_status_t app_profiler_profile(
    app_profiler_t *profiler, app_instruction_t const *instruction);

#endif
