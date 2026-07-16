#include "app/axis.h"
#include "platform/samd21g18a/assert.h"

#include <stdio.h>
#include "app/serial.h"

#define MIN_LOW            (-1000)
#define MAX_HIGH           (1000)
#define UNIT_MAX           (1000000U)
#define STAGE_RANGE_MIN_NM (-6000000)
#define STAGE_RANGE_MAX_NM (6000000)
#define STAGE_TOLERANCE    (300U)

app_axis_status_t
app_axis_init (app_axis_t       *axis,
               int               min,
               int               max,
               uint32_t          unit_nm,
               int               origin_nm,
               app_controller_t *controller)
{
    int min_nm;
    int max_nm;

    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);

    if (min < MIN_LOW)
    {
        return APP_AXIS_STATUS_INIT_ERR_MIN_TOO_LOW;
    }

    if (max > MAX_HIGH)
    {
        return APP_AXIS_STATUS_INIT_ERR_MAX_TOO_HIGH;
    }

    if (min > max)
    {
        return APP_AXIS_STATUS_INIT_ERR_MIN_GREATER_THAN_MAX;
    }

    if (unit_nm < STAGE_TOLERANCE)
    {
        return APP_AXIS_STATUS_INIT_ERR_UNIT_SMALLER_THAN_TOLERANCE;
    }

    if (unit_nm > UNIT_MAX)
    {
        return APP_AXIS_STATUS_INIT_ERR_UNIT_TOO_LARGE;
    }

    if (origin_nm < STAGE_RANGE_MIN_NM || origin_nm > STAGE_RANGE_MAX_NM)
    {
        return APP_AXIS_STATUS_INIT_ERR_ORIGIN_OUTSIDE_RANGE;
    }

    min_nm = origin_nm + (min * (int)unit_nm);
    max_nm = origin_nm + (max * (int)unit_nm);

    if (min_nm <= STAGE_RANGE_MIN_NM || min_nm >= STAGE_RANGE_MAX_NM
        || max_nm <= STAGE_RANGE_MIN_NM || max_nm >= STAGE_RANGE_MAX_NM)
    {
        return APP_AXIS_STATUS_INIT_ERR_BOUNDS_OUTSIDE_RANGE;
    }

    axis->min        = min;
    axis->max        = max;
    axis->unit_nm    = unit_nm;
    axis->origin_nm  = origin_nm;
    axis->current    = 0;
    axis->target     = axis->current;
    axis->controller = controller;

    return APP_AXIS_STATUS_INIT_OK;
}

bool
app_axis_get_stage_moving (app_axis_t const *axis)
{
    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);

    return app_controller_get_stage_moving(axis->controller);
}

size_t
app_axis_num_points (const app_axis_t *axis)
{
    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);

    return (size_t)(axis->max - axis->min) + 1U;
}

void
app_axis_set_target (app_axis_t *axis, int target)
{
    uint16_t value;
    int      target_nm;
        char buf[64];

snprintf(buf, sizeof(buf), "target=%d",
         (unsigned)target);
app_serial_write_line(buf);

    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);
    PLATFORM_SAMD21G18A_ASSERT(target >= axis->min && target <= axis->max);

    axis->target = target;

    if (axis->current == axis->target)
    {
        return;
    }

    target_nm = target * (int)axis->unit_nm + axis->origin_nm;

    // Calculate the analog value of the coordinate.
    value = (uint16_t)((((uint64_t)(target_nm - STAGE_RANGE_MIN_NM))
                        * BOARD_INDIO_ANALOG_OUTPUT_MAX_VALUE)
                       / (STAGE_RANGE_MAX_NM - STAGE_RANGE_MIN_NM));

    app_controller_write_analog_in(axis->controller, value);

    return;
}

void
app_axis_move_start (app_axis_t *axis)
{
    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);

    if (axis->current == axis->target)
    {
        return;
    }

    app_controller_interrupts_enable(axis->controller);

    // NOTE: Setting the state of the stage to moving before pulsing the
    //       controller Trigger IN is important in ensuring accurate state
    //       tracking in the unlikely circumstance that the stage's movement
    //       terminates before the pulse is over, since
    //       `controller_pulse_trigger_in` is blocking.
    app_controller_set_stage_moving(axis->controller, true);
    app_controller_pulse_trigger_in(axis->controller);

    return;
}

void
app_axis_move_end (app_axis_t *axis)
{
    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);

    if (axis->current == axis->target)
    {
        return;
    }

    axis->current = axis->target;

    app_controller_interrupts_disable(axis->controller);

    return;
}
