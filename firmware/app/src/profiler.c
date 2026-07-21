#include "app/profiler.h"
#include "app/axis.h"
#include "app/instruction.h"
#include "app/path.h"
#include "app/pulse_receiver.h"
#include "app/pulse_tracker.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/time.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// TODO: Tighten delays after entire pipeline is tested.
#define TARGET_SET_DEBOUNCE_TIME_USEC (1000U)
#define AXES_TIMEOUT_MSEC             (10000U)
#define PULSE_COUNTER_TIMEOUT_MSEC    (10000U)

static app_profiler_status_t
profile_mode_point_count (app_profiler_t          *profiler,
                          app_instruction_t const *instruction)
{
    app_profiler_status_t status;

    app_axis_t          x;
    app_axis_t          y;
    app_pulse_tracker_t tracker;

    app_path_position_t *path;
    size_t               path_size;

    if (app_axis_init(&x,
                      instruction->x_min,
                      instruction->x_max,
                      instruction->x_unit_nm,
                      instruction->x_origin_nm,
                      profiler->x_controller)
        != APP_AXIS_STATUS_INIT_OK)
    {
        status = APP_PROFILER_STATUS_ERR_X_AXIS_INIT;
    }

    if (app_axis_init(&y,
                      instruction->y_min,
                      instruction->y_max,
                      instruction->y_unit_nm,
                      instruction->y_origin_nm,
                      profiler->y_controller)
        != APP_AXIS_STATUS_INIT_OK)
    {
        status = APP_PROFILER_STATUS_ERR_Y_AXIS_INIT;
    }

    app_pulse_tracker_init(
        &tracker, profiler->receiver, APP_PULSE_TRACKER_MODE_RELAY_AND_COUNT);

    // Generate the full raster.
    path = app_path_modified_raster(
        &x, &y, &(profiler->prev_raster_direction), false, &path_size);

    if (path == NULL)
    {
        return APP_PROFILER_STATUS_ERR_PATH_NOT_GENERATED;
    }

    for (size_t i = 0; i < path_size; i++)
    {
        uint32_t start_msec;

        if (app_axis_set_target(&x, path[i].x) != APP_AXIS_STATUS_TARGET_OK
            || app_axis_set_target(&y, path[i].y) != APP_AXIS_STATUS_TARGET_OK)
        {
            status = APP_PROFILER_STATUS_ERR_TARGET;
            goto cleanup;
        }

        platform_samd21g18a_time_sleep_usec(TARGET_SET_DEBOUNCE_TIME_USEC);

        // Move to the next point.
        app_axis_move_start(&x);
        app_axis_move_start(&y);

        start_msec = platform_samd21g18a_time_msec();

        while (app_axis_get_stage_moving(&x) || app_axis_get_stage_moving(&y))
        {
            profiler->task();

            if ((platform_samd21g18a_time_msec() - start_msec)
                > AXES_TIMEOUT_MSEC)
            {
                status = APP_PROFILER_STATUS_ERR_AXES_TIMEOUT;
                goto cleanup;
            }
        }

        app_axis_move_end(&x);
        app_axis_move_end(&y);

        // Count pulses.
        app_pulse_tracker_start(&tracker);

        start_msec = platform_samd21g18a_time_msec();

        while (app_pulse_tracker_get_count(&tracker) < instruction->num_pulses)
        {
            profiler->task();

            if ((platform_samd21g18a_time_msec() - start_msec)
                > PULSE_COUNTER_TIMEOUT_MSEC)
            {
                status = APP_PROFILER_STATUS_ERR_PULSE_COUNTER_TIMEOUT;
                goto cleanup;
            }
        }

        app_pulse_tracker_end(&tracker);

        platform_samd21g18a_time_sleep_usec(instruction->posttrigger_time_us);
    }

    status = APP_PROFILER_STATUS_OK;

cleanup:
    free(path);
    path = NULL;

    return status;
}

static app_profiler_status_t
profile_mode_point_time (app_profiler_t          *profiler,
                         app_instruction_t const *instruction)
{
    app_profiler_status_t status;

    app_axis_t          x;
    app_axis_t          y;
    app_pulse_tracker_t tracker;

    app_path_position_t *path;
    size_t               path_size;

    if (app_axis_init(&x,
                      instruction->x_min,
                      instruction->x_max,
                      instruction->x_unit_nm,
                      instruction->x_origin_nm,
                      profiler->x_controller)
        != APP_AXIS_STATUS_INIT_OK)
    {
        status = APP_PROFILER_STATUS_ERR_X_AXIS_INIT;
    }

    if (app_axis_init(&y,
                      instruction->y_min,
                      instruction->y_max,
                      instruction->y_unit_nm,
                      instruction->y_origin_nm,
                      profiler->y_controller)
        != APP_AXIS_STATUS_INIT_OK)
    {
        status = APP_PROFILER_STATUS_ERR_Y_AXIS_INIT;
    }

    app_pulse_tracker_init(
        &tracker, profiler->receiver, APP_PULSE_TRACKER_MODE_LAZY);

    // Generate the full raster.
    path = app_path_modified_raster(
        &x, &y, &(profiler->prev_raster_direction), false, &path_size);

    if (path == NULL)
    {
        return APP_PROFILER_STATUS_ERR_PATH_NOT_GENERATED;
    }

    for (size_t i = 0; i < path_size; i++)
    {
        uint32_t start_msec;

        if (app_axis_set_target(&x, path[i].x) != APP_AXIS_STATUS_TARGET_OK
            || app_axis_set_target(&y, path[i].y) != APP_AXIS_STATUS_TARGET_OK)
        {
            status = APP_PROFILER_STATUS_ERR_TARGET;
            goto cleanup;
        }

        platform_samd21g18a_time_sleep_usec(TARGET_SET_DEBOUNCE_TIME_USEC);

        // Move to the next point.
        app_axis_move_start(&x);
        app_axis_move_start(&y);

        start_msec = platform_samd21g18a_time_msec();

        while (app_axis_get_stage_moving(&x) || app_axis_get_stage_moving(&y))
        {
            profiler->task();

            if ((platform_samd21g18a_time_msec() - start_msec)
                > AXES_TIMEOUT_MSEC)
            {
                status = APP_PROFILER_STATUS_ERR_AXES_TIMEOUT;
                goto cleanup;
            }
        }

        app_axis_move_end(&x);
        app_axis_move_end(&y);

        app_pulse_tracker_relay_pulse(&tracker);

        platform_samd21g18a_time_sleep_msec(instruction->wait_time_ms);
    }

    status = APP_PROFILER_STATUS_OK;

cleanup:
    free(path);
    path = NULL;

    return status;
}

static app_profiler_status_t
profile_mode_continuous (app_profiler_t          *profiler,
                         app_instruction_t const *instruction)
{
    app_profiler_status_t status;

    app_axis_t          x;
    app_axis_t          y;
    app_pulse_tracker_t tracker;

    app_path_position_t *path;
    size_t               path_size;

    if (app_axis_init(&x,
                      instruction->x_min,
                      instruction->x_max,
                      instruction->x_unit_nm,
                      instruction->x_origin_nm,
                      profiler->x_controller)
        != APP_AXIS_STATUS_INIT_OK)
    {
        status = APP_PROFILER_STATUS_ERR_X_AXIS_INIT;
    }

    if (app_axis_init(&y,
                      instruction->y_min,
                      instruction->y_max,
                      instruction->y_unit_nm,
                      instruction->y_origin_nm,
                      profiler->y_controller)
        != APP_AXIS_STATUS_INIT_OK)
    {
        status = APP_PROFILER_STATUS_ERR_Y_AXIS_INIT;
    }

    app_pulse_tracker_init(
        &tracker, profiler->receiver, APP_PULSE_TRACKER_MODE_RELAY);

    // Generate only the corners of the raster.
    path = app_path_modified_raster(
        &x, &y, &(profiler->prev_raster_direction), true, &path_size);

    if (path == NULL)
    {
        return APP_PROFILER_STATUS_ERR_PATH_NOT_GENERATED;
    }

    app_pulse_tracker_start(&tracker);

    for (size_t i = 0; i < path_size; i++)
    {
        uint32_t start_msec;

        if (app_axis_set_target(&x, path[i].x) != APP_AXIS_STATUS_TARGET_OK
            || app_axis_set_target(&y, path[i].y) != APP_AXIS_STATUS_TARGET_OK)
        {
            status = APP_PROFILER_STATUS_ERR_TARGET;
            goto cleanup;
        }

        platform_samd21g18a_time_sleep_usec(TARGET_SET_DEBOUNCE_TIME_USEC);

        // Move to the next point.
        app_axis_move_start(&x);
        app_axis_move_start(&y);

        start_msec = platform_samd21g18a_time_msec();

        while (app_axis_get_stage_moving(&x) || app_axis_get_stage_moving(&y))
        {
            profiler->task();

            if ((platform_samd21g18a_time_msec() - start_msec)
                > AXES_TIMEOUT_MSEC)
            {
                status = APP_PROFILER_STATUS_ERR_AXES_TIMEOUT;
                goto cleanup;
            }
        }

        app_axis_move_end(&x);
        app_axis_move_end(&y);
    }

    app_pulse_tracker_end(&tracker);

cleanup:
    free(path);
    path = NULL;

    return status;
}

app_profiler_status_t
app_profiler_init (app_profiler_t       *profiler,
                   app_controller_t     *x_controller,
                   app_controller_t     *y_controller,
                   app_pulse_receiver_t *receiver,
                   app_profiler_task_t   task)
{
    PLATFORM_SAMD21G18A_ASSERT(profiler != NULL);
    PLATFORM_SAMD21G18A_ASSERT(x_controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(y_controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(receiver != NULL);
    PLATFORM_SAMD21G18A_ASSERT(task != NULL);

    profiler->x_controller          = x_controller;
    profiler->y_controller          = y_controller;
    profiler->receiver              = receiver;
    profiler->task                  = task;
    profiler->prev_raster_direction = APP_PATH_RASTER_DIRECTION_HORIZONTAL;

    return APP_PROFILER_STATUS_OK;
}

app_profiler_status_t
app_profiler_profile (app_profiler_t          *profiler,
                      app_instruction_t const *instruction)
{
    PLATFORM_SAMD21G18A_ASSERT(profiler != NULL);
    PLATFORM_SAMD21G18A_ASSERT(instruction != NULL);

    switch (instruction->mode)
    {
        case APP_INSTRUCTION_MODE_POINT_COUNT:
            return profile_mode_point_count(profiler, instruction);
            break;

        case APP_INSTRUCTION_MODE_POINT_TIME:
            return profile_mode_point_time(profiler, instruction);
            break;

        case APP_INSTRUCTION_MODE_CONTINUOUS:
            return profile_mode_continuous(profiler, instruction);
            break;

        default:
            PLATFORM_SAMD21G18A_ASSERT(false);
    }

    return APP_PROFILER_STATUS_ERR;
}
