/**
 * @file profiler.c
 * @brief Implementation of core profiling logic.
 */
#include "app/profiler.h"
#include "app/axis.h"
#include "app/parameters.h"
#include "app/path.h"
#include "app/relay.h"
#include "app/serial.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/time.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define GO_TO_POINT_COMMAND         ("{\"cmd\":\"go_to_point\"}")
#define IN_TIMEOUT_MS               (5000u)
#define IN_POLL_INTERVAL_US         (100u)
#define TARGET_SET_DEBOUNCE_TIME_US (500u)
#define AXES_TIMEOUT_MS             (10000u)
#define RELAY_TIMEOUT_MS            (10000u)

static bool poll_in(profiler_t const *profiler, char const *expected);
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
               serial_buf_t   *in,
               profiler_task_t task)
{
    ASSERT(profiler != NULL);
    ASSERT(x_controller != NULL);
    ASSERT(y_controller != NULL);
    ASSERT(relay != NULL);
    ASSERT(in != NULL);
    ASSERT(task != NULL);

    profiler->x_controller          = x_controller;
    profiler->y_controller          = y_controller;
    profiler->relay                 = relay;
    profiler->prev_raster_direction = PATH_RASTER_DIRECTION_HORIZONTAL;
    profiler->in                    = in;
    profiler->task                  = task;

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

/** @brief Poll the serial connection for a message. */
static bool
poll_in (profiler_t const *profiler, char const *expected)
{
    uint32_t start_ms = time_get_ms();

    while (time_get_ms() - start_ms < IN_TIMEOUT_MS)
    {
        profiler->task();

        if (serial_read_line(profiler->in) == SERIAL_STATUS_OK_LINE_RECEIVED
            && strcmp(profiler->in->str, expected) == 0)
        {
            return true;
        }

        time_sleep_us(IN_POLL_INTERVAL_US);
    }

    return false;
}

/** @brief Profile a beam in `POINT_COUNT` mode. */
static profiler_status_t
profile_mode_point_count (profiler_t *profiler, parameters_t const *parameters)
{
    profiler_status_t status = PROFILER_STATUS_ERR;

    axis_t x;
    axis_t y;

    if (axis_init(&x,
                  parameters->x_min,
                  parameters->x_max,
                  parameters->x_unit_nm,
                  parameters->x_origin_nm,
                  profiler->x_controller)
        != AXIS_STATUS_OK)
    {
        return PROFILER_STATUS_ERR_X_AXIS_INIT;
    }

    if (axis_init(&y,
                  parameters->y_min,
                  parameters->y_max,
                  parameters->y_unit_nm,
                  parameters->y_origin_nm,
                  profiler->y_controller)
        != AXIS_STATUS_OK)
    {
        return PROFILER_STATUS_ERR_Y_AXIS_INIT;
    }

    path_t path;

    if (path_init(&path,
                  false,
                  &profiler->prev_raster_direction,
                  parameters->x_min,
                  parameters->x_max,
                  parameters->y_min,
                  parameters->y_max)
        != PATH_STATUS_OK)
    {
        return PROFILER_STATUS_ERR_PATH_INIT;
    };

    path_coords_t coords;
    path_phase_t  phase;

    do
    {
        coords = path_next_coords(&path);
        phase  = path_get_phase(&path);

        if ((phase != PATH_PHASE_RETURNING)
            && (!poll_in(profiler, GO_TO_POINT_COMMAND)))
        {
            return PROFILER_STATUS_ERR_MOVE_COMMAND_TIMEOUT;
        }

        if (axis_set_target(&x, coords.x) != AXIS_STATUS_OK
            || axis_set_target(&y, coords.y) != AXIS_STATUS_OK)
        {
            status = PROFILER_STATUS_ERR_TARGET;
            goto cleanup;
        }

        time_sleep_us(TARGET_SET_DEBOUNCE_TIME_US);

        // Move to the next point.
        axis_move_start(&x);
        axis_move_start(&y);

        uint32_t start_ms = time_get_ms();

        while (axis_get_stage_moving(&x) || axis_get_stage_moving(&y))
        {
            profiler->task();

            if ((time_get_ms() - start_ms) >= AXES_TIMEOUT_MS)
            {
                status = PROFILER_STATUS_ERR_AXES_TIMEOUT;
                goto cleanup;
            }
        }

        axis_move_end(&x);
        axis_move_end(&y);

        if (phase != PATH_PHASE_RETURNING)
        {
            // Count pulses.
            relay_count_start(profiler->relay);
            relay_pulser_start(profiler->relay);

            start_ms = time_get_ms();

            while (relay_count_get(profiler->relay) < parameters->num_pulses)
            {
                profiler->task();

                if ((time_get_ms() - start_ms) >= RELAY_TIMEOUT_MS)
                {
                    status = PROFILER_STATUS_ERR_RELAY_TIMEOUT;
                    goto cleanup;
                }
            }

            relay_count_end(profiler->relay);
            relay_pulser_end(profiler->relay);

            uint32_t start_us = time_get_us();

            while (time_get_us() - start_us < parameters->posttrigger_time_us)
            {
                profiler->task();
            }
        }
    } while (phase == PATH_PHASE_ONGOING);

    return PROFILER_STATUS_OK;

// NOTE: Only idempotent functions should be called here.
cleanup:
    axis_move_abort(&x);
    axis_move_abort(&y);

    relay_pulser_abort(profiler->relay);

    return status;
}

/** @brief Profile a beam in `POINT_TIME` mode. */
static profiler_status_t
profile_mode_point_time (profiler_t *profiler, parameters_t const *parameters)
{
    profiler_status_t status = PROFILER_STATUS_ERR;

    axis_t x;
    axis_t y;

    if (axis_init(&x,
                  parameters->x_min,
                  parameters->x_max,
                  parameters->x_unit_nm,
                  parameters->x_origin_nm,
                  profiler->x_controller)
        != AXIS_STATUS_OK)
    {
        return PROFILER_STATUS_ERR_X_AXIS_INIT;
    }

    if (axis_init(&y,
                  parameters->y_min,
                  parameters->y_max,
                  parameters->y_unit_nm,
                  parameters->y_origin_nm,
                  profiler->y_controller)
        != AXIS_STATUS_OK)
    {
        return PROFILER_STATUS_ERR_Y_AXIS_INIT;
    }

    path_t path;

    if (path_init(&path,
                  false,
                  &profiler->prev_raster_direction,
                  parameters->x_min,
                  parameters->x_max,
                  parameters->y_min,
                  parameters->y_max)
        != PATH_STATUS_OK)
    {
        return PROFILER_STATUS_ERR_PATH_INIT;
    };

    path_coords_t coords;
    path_phase_t  phase;

    do
    {
        coords = path_next_coords(&path);
        phase  = path_get_phase(&path);

        if ((phase != PATH_PHASE_RETURNING)
            && (!poll_in(profiler, GO_TO_POINT_COMMAND)))
        {
            return PROFILER_STATUS_ERR_MOVE_COMMAND_TIMEOUT;
        }

        if (axis_set_target(&x, coords.x) != AXIS_STATUS_OK
            || axis_set_target(&y, coords.y) != AXIS_STATUS_OK)
        {
            status = PROFILER_STATUS_ERR_TARGET;
            goto cleanup;
        }

        time_sleep_us(TARGET_SET_DEBOUNCE_TIME_US);

        // Move to the next point.
        axis_move_start(&x);
        axis_move_start(&y);

        uint32_t start_ms = time_get_ms();

        while (axis_get_stage_moving(&x) || axis_get_stage_moving(&y))
        {
            profiler->task();

            if ((time_get_ms() - start_ms) >= AXES_TIMEOUT_MS)
            {
                status = PROFILER_STATUS_ERR_AXES_TIMEOUT;
                goto cleanup;
            }
        }

        axis_move_end(&x);
        axis_move_end(&y);

        if (phase != PATH_PHASE_RETURNING)
        {
            relay_pulser_retrigger(profiler->relay);

            uint32_t start_us = time_get_us();

            while (time_get_us() - start_us < parameters->wait_time_us)
            {
                profiler->task();
            }
        }
    } while (phase == PATH_PHASE_ONGOING);

    return PROFILER_STATUS_OK;

// NOTE: Only idempotent functions should be called here. No relay functions are
//       called here because the event path is not configured, and pulses are
//       only sent through software re-triggering.
cleanup:
    axis_move_abort(&x);
    axis_move_abort(&y);

    return status;
}

/** @brief Profile a beam in `CONTINUOUS` mode. */
static profiler_status_t
profile_mode_continuous (profiler_t *profiler, parameters_t const *parameters)
{
    profiler_status_t status = PROFILER_STATUS_ERR;

    axis_t x;
    axis_t y;

    if (axis_init(&x,
                  parameters->x_min,
                  parameters->x_max,
                  parameters->x_unit_nm,
                  parameters->x_origin_nm,
                  profiler->x_controller)
        != AXIS_STATUS_OK)
    {
        return PROFILER_STATUS_ERR_X_AXIS_INIT;
    }

    if (axis_init(&y,
                  parameters->y_min,
                  parameters->y_max,
                  parameters->y_unit_nm,
                  parameters->y_origin_nm,
                  profiler->y_controller)
        != AXIS_STATUS_OK)
    {
        return PROFILER_STATUS_ERR_Y_AXIS_INIT;
    }

    path_t path;

    if (path_init(&path,
                  true,
                  &profiler->prev_raster_direction,
                  parameters->x_min,
                  parameters->x_max,
                  parameters->y_min,
                  parameters->y_max)
        != PATH_STATUS_OK)
    {
        return PROFILER_STATUS_ERR_PATH_INIT;
    };

    relay_pulser_start(profiler->relay);

    path_coords_t coords;
    path_phase_t  phase;

    do
    {
        coords = path_next_coords(&path);
        phase  = path_get_phase(&path);

        if (axis_set_target(&x, coords.x) != AXIS_STATUS_OK
            || axis_set_target(&y, coords.y) != AXIS_STATUS_OK)
        {
            status = PROFILER_STATUS_ERR_TARGET;
            goto cleanup;
        }

        time_sleep_us(TARGET_SET_DEBOUNCE_TIME_US);

        // Move to the next point.
        axis_move_start(&x);
        axis_move_start(&y);

        uint32_t start_ms = time_get_ms();

        while (axis_get_stage_moving(&x) || axis_get_stage_moving(&y))
        {
            profiler->task();

            if ((time_get_ms() - start_ms) >= AXES_TIMEOUT_MS)
            {
                status = PROFILER_STATUS_ERR_AXES_TIMEOUT;
                goto cleanup;
            }
        }

        axis_move_end(&x);
        axis_move_end(&y);
    } while (phase == PATH_PHASE_ONGOING);

    relay_pulser_end(profiler->relay);

    return PROFILER_STATUS_OK;

// NOTE: Only idempotent functions should be called here.
cleanup:
    axis_move_abort(&x);
    axis_move_abort(&y);

    relay_pulser_abort(profiler->relay);

    return status;
}
