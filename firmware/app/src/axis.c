#include "app/axis.h"
#include "platform/samd21g18a/assert.h"

#define STAGE_RANGE_MIN_NM (0)
#define STAGE_RANGE_MAX_NM (12000000)
#define STAGE_TOLERANCE    (300U)

app_axis_status_t
app_axis_init (app_axis_t       *axis,
               int               min,
               int               max,
               uint32_t          unit_nm,
               int               origin_position_nm,
               app_controller_t *controller)
{
    int min_position_nm;
    int max_position_nm;

    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);
    PLATFORM_SAMD21G18A_ASSERT(controller != NULL);

    if (min > max)
    {
        return APP_AXIS_STATUS_INIT_MIN_GREATER_THAN_MAX;
    }

    if (unit_nm < STAGE_TOLERANCE)
    {
        return APP_AXIS_STATUS_INIT_UNIT_SMALLER_THAN_TOLERANCE;
    }

    min_position_nm = origin_position_nm + (min * (int)unit_nm);
    max_position_nm = origin_position_nm + (max * (int)unit_nm);

    if (min_position_nm <= STAGE_RANGE_MIN_NM
        || min_position_nm >= STAGE_RANGE_MAX_NM
        || max_position_nm <= STAGE_RANGE_MIN_NM
        || max_position_nm >= STAGE_RANGE_MAX_NM)
    {
        return APP_AXIS_STATUS_INIT_BOUND_OUTSIDE_RANGE;
    }

    axis->min                = min;
    axis->max                = max;
    axis->unit_nm            = unit_nm;
    axis->origin_position_nm = origin_position_nm;
    axis->current            = 0;
    axis->target             = axis->current;
    axis->controller         = controller;

    return APP_AXIS_STATUS_INIT_OK;
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
    int      target_position_nm;

    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);
    PLATFORM_SAMD21G18A_ASSERT(target >= axis->min && target <= axis->max);

    axis->target = target;

    if (axis->current == axis->target)
    {
        return;
    }

    target_position_nm = target * (int)axis->unit_nm + axis->origin_position_nm;

    // Calculate the analog value of the coordinate.
    value = (uint16_t)(((uint64_t)((uint32_t)(target_position_nm
                                              - STAGE_RANGE_MIN_NM))
                        * (BOARD_INDIO_ANALOG_OUTPUT_MAX_VALUE
                           - BOARD_INDIO_ANALOG_OUTPUT_MIN_VALUE))
                           / (STAGE_RANGE_MAX_NM - STAGE_RANGE_MIN_NM)
                       + BOARD_INDIO_ANALOG_OUTPUT_MIN_VALUE);

    app_controller_write_analog_in(axis->controller, value);
}

void
app_axis_start_move (app_axis_t *axis)
{
    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);

    if (axis->current == axis->target)
    {
        return;
    }

    // NOTE: Setting the state of the stage to moving before pulsing the
    //       controller Trigger IN is important in ensuring accurate state
    //       tracking in the unlikely circumstance that the stage's movement
    //       terminates before the pulse is over, since
    //       `controller_pulse_trigger_in` is blocking.
    app_controller_set_stage_moving(axis->controller, true);
    app_controller_pulse_trigger_in(axis->controller);
}

bool
app_axis_stage_moving (app_axis_t *axis)
{
    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);

    return app_controller_get_stage_moving(axis->controller);
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
}
