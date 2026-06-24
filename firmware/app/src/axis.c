#include "app/axis.h"
#include "platform/samd21g18a/dac.h"

void
app_axis_init (app_axis_t       *axis,
               int               min,
               int               max,
               int               unit_nm,
               uint16_t          origin_analog_val,
               app_controller_t *controller)
{
    axis->min     = min;
    axis->max     = max;
    axis->unit_nm = unit_nm;

    axis->origin_analog_val = origin_analog_val;

    axis->current    = 0;
    axis->target     = axis->current;
    axis->controller = controller;
}

size_t
app_axis_num_points (const app_axis_t *axis)
{
    return (size_t)(axis->max - axis->min) + 1u;
}

void
app_axis_set_target (app_axis_t *axis, int target)
{
    axis->target = target;

    if (axis->current == axis->target)
    {
        return;
    }

    // Calculate the analog value of the coordinate.
    uint16_t analog_val = (target - axis->min)
                              * (PLATFORM_SAMD21G18A_DAC_MAX_VALUE
                                 - PLATFORM_SAMD21G18A_DAC_MIN_VALUE)
                              / (axis->max - axis->min)
                          + PLATFORM_SAMD21G18A_DAC_MIN_VALUE
                          + axis->origin_analog_val;

    app_controller_write_analog_in(axis->controller, analog_val);
}

void
app_axis_start_move (app_axis_t *axis)
{
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
    return app_controller_get_stage_moving(axis->controller);
}

void
app_axis_move_end (app_axis_t *axis)
{
    if (axis->current == axis->target)
    {
        return;
    }

    axis->current = axis->target;
}
