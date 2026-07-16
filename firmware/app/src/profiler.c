#include "app/profiler.h"
#include "app/axis.h"
#include "app/path.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/time.h"
#include <stddef.h>
#include <stdlib.h>

// TODO: Tighten delays after entire pipeline is tested.
#define TARGET_SET_DEBOUNCE_TIME_USEC (1000U)

app_profiler_status_t
app_profiler_init (app_profiler_t      *profiler,
                   app_controller_t    *x_controller,
                   app_controller_t    *y_controller,
                   app_pulse_counter_t *pulse_counter,
                   app_profiler_task_t  task)
{
    PLATFORM_SAMD21G18A_ASSERT(profiler != NULL);
    PLATFORM_SAMD21G18A_ASSERT(x_controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(y_controller != NULL);
    PLATFORM_SAMD21G18A_ASSERT(pulse_counter != NULL);
    PLATFORM_SAMD21G18A_ASSERT(task != NULL);

    profiler->x_controller          = x_controller;
    profiler->y_controller          = y_controller;
    profiler->pulse_counter         = pulse_counter;
    profiler->task                  = task;
    profiler->prev_raster_direction = APP_PATH_RASTER_DIRECTION_HORIZONTAL;

    return APP_PROFILER_STATUS_OK;
}

app_profiler_status_t
app_profiler_profile (app_profiler_t          *profiler,
                      app_instruction_t const *instruction)
{
    app_axis_t x;
    app_axis_t y;

    app_path_position_t *path;
    size_t               path_size;

    PLATFORM_SAMD21G18A_ASSERT(profiler != NULL);
    PLATFORM_SAMD21G18A_ASSERT(instruction != NULL);

    if (app_axis_init(&x,
                      instruction->x_min,
                      instruction->x_max,
                      instruction->x_unit_nm,
                      instruction->x_origin_nm,
                      profiler->x_controller)
        != APP_AXIS_STATUS_INIT_OK)
    {
        return APP_PROFILER_STATUS_ERR;
    }

    if (app_axis_init(&y,
                      instruction->y_min,
                      instruction->y_max,
                      instruction->y_unit_nm,
                      instruction->y_origin_nm,
                      profiler->y_controller)
        != APP_AXIS_STATUS_INIT_OK)
    {
        return APP_PROFILER_STATUS_ERR;
    }

    path = app_path_modified_raster(
        &x, &y, &(profiler->prev_raster_direction), &path_size);

    if (path == NULL)
    {
        return APP_PROFILER_STATUS_ERR;
    }

    PLATFORM_SAMD21G18A_ASSERT(path != NULL);

    for (size_t i = 0; i < path_size; i++)
    {
        app_axis_set_target(&x, path[i].x);
        app_axis_set_target(&y, path[i].y);

        platform_samd21g18a_time_sleep_usec(TARGET_SET_DEBOUNCE_TIME_USEC);

        // Move to the next point.
        app_axis_move_start(&x);
        app_axis_move_start(&y);

        while (app_axis_get_stage_moving(&x) || app_axis_get_stage_moving(&y))
        {
            profiler->task();
        }

        app_axis_move_end(&x);
        app_axis_move_end(&y);

        // Count pulses.
        app_pulse_counter_start(profiler->pulse_counter);

        while (app_pulse_counter_get_count(profiler->pulse_counter)
               < instruction->num_pulses)
        {
            profiler->task();
        }

        app_pulse_counter_end(profiler->pulse_counter);

        platform_samd21g18a_time_sleep_usec(instruction->posttrigger_time_us);
    }

    free(path);
    path = NULL;

    return APP_PROFILER_STATUS_OK;
}
