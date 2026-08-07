/**
 * @file axis.c
 * @brief Implementation of coordinate system abstraction.
 */
#include "app/axis.h"
#include "app/controller.h"
#include "platform/samd21g18a/assert.h"

#define MIN_MIN            (-1000)
#define MAX_MAX            (1000)
#define UNIT_MAX_NM        (1000000u)
#define STAGE_RANGE_MIN_NM (-6000000)
#define STAGE_RANGE_MAX_NM (6000000)
#define STAGE_MIN_STEP_NM  (300u)

axis_status_t
axis_init (axis_t       *axis,
           int           min,
           int           max,
           uint32_t      unit_nm,
           int           origin_nm,
           controller_t *controller)
{
    ASSERT(axis != NULL);
    ASSERT(controller != NULL);

    if (min < MIN_MIN)
    {
        return AXIS_STATUS_ERR_MIN_TOO_SMALL;
    }

    if (max > MAX_MAX)
    {
        return AXIS_STATUS_ERR_MAX_TOO_LARGE;
    }

    if (min > max)
    {
        return AXIS_STATUS_ERR_MIN_GREATER_THAN_MAX;
    }

    if (unit_nm < STAGE_MIN_STEP_NM)
    {
        return AXIS_STATUS_ERR_UNIT_SMALLER_THAN_MIN_STEP;
    }

    if (unit_nm > UNIT_MAX_NM)
    {
        return AXIS_STATUS_ERR_UNIT_TOO_LARGE;
    }

    if (origin_nm < STAGE_RANGE_MIN_NM || origin_nm > STAGE_RANGE_MAX_NM)
    {
        return AXIS_STATUS_ERR_ORIGIN_OUTSIDE_RANGE;
    }

    int min_nm = origin_nm + (min * (int)unit_nm);
    int max_nm = origin_nm + (max * (int)unit_nm);

    if (min_nm <= STAGE_RANGE_MIN_NM || min_nm >= STAGE_RANGE_MAX_NM
        || max_nm <= STAGE_RANGE_MIN_NM || max_nm >= STAGE_RANGE_MAX_NM)
    {
        return AXIS_STATUS_ERR_BOUNDS_OUTSIDE_RANGE;
    }

    axis->min        = min;
    axis->max        = max;
    axis->unit_nm    = unit_nm;
    axis->origin_nm  = origin_nm;
    axis->controller = controller;

    return AXIS_STATUS_OK;
}

bool
axis_get_stage_moving (axis_t const *axis)
{
    ASSERT(axis != NULL);
    ASSERT(axis->controller != NULL);

    return controller_get_stage_moving(axis->controller);
}

axis_status_t
axis_set_target (axis_t *axis, int target)
{
    ASSERT(axis != NULL);
    ASSERT(target >= axis->min && target <= axis->max);

    int target_nm = target * (int)axis->unit_nm + axis->origin_nm;

    // Calculate the analog value of the coordinate.
    uint16_t value = (uint16_t)((((uint64_t)(target_nm - STAGE_RANGE_MIN_NM))
                                 * ANALOG_OUTPUT_MAX_VALUE)
                                / (STAGE_RANGE_MAX_NM - STAGE_RANGE_MIN_NM));

    if (controller_write_analog_in(axis->controller, value)
        != CONTROLLER_STATUS_ANALOG_IN_OK)
    {
        return AXIS_STATUS_ERR_CONTROLLER;
    }

    return AXIS_STATUS_OK;
}

void
axis_move_start (axis_t *axis)
{
    ASSERT(axis != NULL);
    ASSERT(axis->controller != NULL);

    controller_interrupt_enable(axis->controller);

    // NOTE: Setting the state of the stage to moving before pulsing the
    //       controller Trigger IN is important in ensuring accurate state
    //       tracking in the unlikely circumstance that the stage's movement
    //       terminates before the pulse is over, since
    //       `controller_pulse_trigger_in` is blocking.
    controller_set_stage_moving(axis->controller, true);
    controller_pulse_trigger_in(axis->controller);

    return;
}

void
axis_move_end (axis_t *axis)
{
    ASSERT(axis != NULL);
    ASSERT(axis->controller != NULL);

    controller_interrupt_disable(axis->controller);

    return;
}
