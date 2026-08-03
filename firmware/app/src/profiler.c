/**
 * @file profiler.c
 * @brief Implementation of core profiling logic.
 */
#include "app/profiler.h"
#include "app/axis.h"
#include "app/parameters.h"
#include "app/path.h"
#include "app/relay.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/time.h"
#include <stdbool.h>
#include <stddef.h>

#define TARGET_SET_DEBOUNCE_TIME_USEC (1000u)
#define AXES_TIMEOUT_MSEC             (10000u)
#define PULSE_COUNTER_TIMEOUT_MSEC    (10000u)

static profiler_status_t profile_mode_point_count(
    profiler_t *profiler, parameters_t const *parameters);
static profiler_status_t profile_mode_point_time(
    profiler_t *profiler, parameters_t const *parameters);
static profiler_status_t profile_mode_continuous(
    profiler_t *profiler, parameters_t const *parameters);

void
profiler_init (profiler_t     *profiler,
               controller_t   *x_controller,
               controller_t   *y_controller,
               relay_t        *relay,
               profiler_task_t task)
{
    ASSERT(profiler != NULL);
    ASSERT(x_controller != NULL);
    ASSERT(y_controller != NULL);
    ASSERT(relay != NULL);
    ASSERT(task != NULL);

    profiler->x_controller          = x_controller;
    profiler->y_controller          = y_controller;
    profiler->relay                 = relay;
    profiler->task                  = task;
    profiler->prev_raster_direction = PATH_RASTER_DIRECTION_HORIZONTAL;

    return;
}

profiler_status_t
profiler_profile (profiler_t *profiler, parameters_t const *parameters)
{
    ASSERT(profiler != NULL);
    ASSERT(parameters != NULL);

    switch (parameters->mode)
    {
        case PARAMETERS_MODE_POINT_COUNT:
            return profile_mode_point_count(profiler, parameters);
            break;

        case PARAMETERS_MODE_POINT_TIME:
            return profile_mode_point_time(profiler, parameters);
            break;

        case PARAMETERS_MODE_CONTINUOUS:
            return profile_mode_continuous(profiler, parameters);
            break;

        case PARAMETERS_MODE_COUNT:
            __builtin_unreachable();

        default:
            ASSERT(false);
    }

    return PROFILER_STATUS_ERR;
}

/** @brief Profile a beam in `POINT_COUNT` mode. */
static profiler_status_t
profile_mode_point_count (profiler_t *profiler, parameters_t const *parameters)
{
    profiler_status_t status = PROFILER_STATUS_ERR;

    axis_t x;
    axis_t y;

    path_position_t *path;
    size_t           path_size;

    if (axis_init(&x,
                  parameters->x_min,
                  parameters->x_max,
                  parameters->x_unit_nm,
                  parameters->x_origin_nm,
                  profiler->x_controller)
        != AXIS_STATUS_INIT_OK)
    {
        return PROFILER_STATUS_ERR_X_AXIS_INIT;
    }

    if (axis_init(&y,
                  parameters->y_min,
                  parameters->y_max,
                  parameters->y_unit_nm,
                  parameters->y_origin_nm,
                  profiler->y_controller)
        != AXIS_STATUS_INIT_OK)
    {
        return PROFILER_STATUS_ERR_Y_AXIS_INIT;
    }

    // Generate the full raster.
    if (path_modified_raster(&x,
                             &y,
                             &(profiler->prev_raster_direction),
                             false,
                             &path,
                             &path_size)
        == PATH_STATUS_ERR)
    {
        return PROFILER_STATUS_ERR_PATH_NOT_GENERATED;
    }

    for (size_t i = 0; i < path_size; i++)
    {
        uint32_t start_msec;
        uint32_t start_usec;

        if (axis_set_target(&x, path[i].x) != AXIS_STATUS_TARGET_OK
            || axis_set_target(&y, path[i].y) != AXIS_STATUS_TARGET_OK)
        {
            status = PROFILER_STATUS_ERR_TARGET;
            goto cleanup;
        }

        time_sleep_usec(TARGET_SET_DEBOUNCE_TIME_USEC);

        // Move to the next point.
        axis_move_start(&x);
        axis_move_start(&y);

        start_msec = time_msec();

        while (axis_get_stage_moving(&x) || axis_get_stage_moving(&y))
        {
            profiler->task();

            if ((time_msec() - start_msec) > AXES_TIMEOUT_MSEC)
            {
                status = PROFILER_STATUS_ERR_AXES_TIMEOUT;
                goto cleanup;
            }
        }

        axis_move_end(&x);
        axis_move_end(&y);

        // Count pulses.
        relay_count_start(profiler->relay);
        relay_pulser_event_start(profiler->relay);

        start_msec = time_msec();

        while (relay_count_get(profiler->relay) < parameters->num_pulses)
        {
            profiler->task();

            if ((time_msec() - start_msec) >= PULSE_COUNTER_TIMEOUT_MSEC)
            {
                status = PROFILER_STATUS_ERR_PULSE_COUNTER_TIMEOUT;
                goto cleanup;
            }
        }

        relay_count_end(profiler->relay);
        relay_pulser_event_end(profiler->relay);

        start_usec = time_usec();

        while (time_usec() - start_usec < parameters->posttrigger_time_us)
        {
            profiler->task();
        }
    }

    status = PROFILER_STATUS_OK;

// NOTE: Only idempotent functions should be called here.
cleanup:
    axis_move_end(&x);
    axis_move_end(&y);

    relay_count_end(profiler->relay);
    relay_pulser_event_end(profiler->relay);

    return status;
}

/** @brief Profile a beam in `POINT_TIME` mode. */
static profiler_status_t
profile_mode_point_time (profiler_t *profiler, parameters_t const *parameters)
{
    profiler_status_t status = PROFILER_STATUS_ERR;

    axis_t x;
    axis_t y;

    path_position_t *path;
    size_t           path_size;

    if (axis_init(&x,
                  parameters->x_min,
                  parameters->x_max,
                  parameters->x_unit_nm,
                  parameters->x_origin_nm,
                  profiler->x_controller)
        != AXIS_STATUS_INIT_OK)
    {
        return PROFILER_STATUS_ERR_X_AXIS_INIT;
    }

    if (axis_init(&y,
                  parameters->y_min,
                  parameters->y_max,
                  parameters->y_unit_nm,
                  parameters->y_origin_nm,
                  profiler->y_controller)
        != AXIS_STATUS_INIT_OK)
    {
        return PROFILER_STATUS_ERR_Y_AXIS_INIT;
    }

    // Generate the full raster.
    if (path_modified_raster(&x,
                             &y,
                             &(profiler->prev_raster_direction),
                             false,
                             &path,
                             &path_size)
        == PATH_STATUS_ERR)
    {
        status = PROFILER_STATUS_ERR_PATH_NOT_GENERATED;
        goto cleanup;
    }

    for (size_t i = 0; i < path_size; i++)
    {
        uint32_t start_msec;
        uint32_t start_usec;

        if (axis_set_target(&x, path[i].x) != AXIS_STATUS_TARGET_OK
            || axis_set_target(&y, path[i].y) != AXIS_STATUS_TARGET_OK)
        {
            status = PROFILER_STATUS_ERR_TARGET;
            goto cleanup;
        }

        time_sleep_usec(TARGET_SET_DEBOUNCE_TIME_USEC);

        // Move to the next point.
        axis_move_start(&x);
        axis_move_start(&y);

        start_msec = time_msec();

        while (axis_get_stage_moving(&x) || axis_get_stage_moving(&y))
        {
            profiler->task();

            if ((time_msec() - start_msec) >= AXES_TIMEOUT_MSEC)
            {
                status = PROFILER_STATUS_ERR_AXES_TIMEOUT;
                goto cleanup;
            }
        }

        axis_move_end(&x);
        axis_move_end(&y);

        relay_pulser_retrigger(profiler->relay);

        start_usec = time_usec();

        while (time_usec() - start_usec < parameters->wait_time_us)
        {
            profiler->task();
        }
    }

    status = PROFILER_STATUS_OK;

// NOTE: Only idempotent functions should be called here. No relay functions are
//       called here because the event path is not configured, and pulses are
//       only sent through software re-triggering.
cleanup:
    axis_move_end(&x);
    axis_move_end(&y);

    return status;
}

/** @brief Profile a beam in `CONTINUOUS` mode. */
static profiler_status_t
profile_mode_continuous (profiler_t *profiler, parameters_t const *parameters)
{
    profiler_status_t status = PROFILER_STATUS_ERR;

    axis_t x;
    axis_t y;

    path_position_t *path;
    size_t           path_size;

    if (axis_init(&x,
                  parameters->x_min,
                  parameters->x_max,
                  parameters->x_unit_nm,
                  parameters->x_origin_nm,
                  profiler->x_controller)
        != AXIS_STATUS_INIT_OK)
    {
        return PROFILER_STATUS_ERR_X_AXIS_INIT;
    }

    if (axis_init(&y,
                  parameters->y_min,
                  parameters->y_max,
                  parameters->y_unit_nm,
                  parameters->y_origin_nm,
                  profiler->y_controller)
        != AXIS_STATUS_INIT_OK)
    {
        return PROFILER_STATUS_ERR_Y_AXIS_INIT;
    }

    // Generate only the corners of the raster.
    if (path_modified_raster(
            &x, &y, &(profiler->prev_raster_direction), true, &path, &path_size)
        == PATH_STATUS_ERR)
    {
        status = PROFILER_STATUS_ERR_PATH_NOT_GENERATED;
        goto cleanup;
    }

    relay_pulser_event_start(profiler->relay);

    for (size_t i = 0; i < path_size; i++)
    {
        uint32_t start_msec;

        if (axis_set_target(&x, path[i].x) != AXIS_STATUS_TARGET_OK
            || axis_set_target(&y, path[i].y) != AXIS_STATUS_TARGET_OK)
        {
            status = PROFILER_STATUS_ERR_TARGET;
            goto cleanup;
        }

        time_sleep_usec(TARGET_SET_DEBOUNCE_TIME_USEC);

        // Move to the next point.
        axis_move_start(&x);
        axis_move_start(&y);

        start_msec = time_msec();

        while (axis_get_stage_moving(&x) || axis_get_stage_moving(&y))
        {
            profiler->task();

            if ((time_msec() - start_msec) >= AXES_TIMEOUT_MSEC)
            {
                status = PROFILER_STATUS_ERR_AXES_TIMEOUT;
                goto cleanup;
            }
        }

        axis_move_end(&x);
        axis_move_end(&y);
    }

    relay_count_end(profiler->relay);

    status = PROFILER_STATUS_OK;

// NOTE: Only idempotent functions should be called here.
cleanup:
    axis_move_end(&x);
    axis_move_end(&y);

    relay_count_end(profiler->relay);

    return status;
}
