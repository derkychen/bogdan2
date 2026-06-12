#include "axis.h"
#include "config.h"

void
axis_init (Axis       *axis,
           int         min,
           int         max,
           int         unit_nm,
           uint16_t    origin_analog_val,
           Controller *controller)
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
axis_num_points (const Axis *axis)
{
    return (size_t)(axis->max - axis->min) + 1u;
}

void
axis_set_target (Axis *axis, int target)
{
    axis->target = target;

    if (axis->current == axis->target)
    {
        return;
    }

    // Calculate the analog value of the coordinate.
    uint16_t analog_val = (target - axis->min) * (PWM_MAX_VAL - PWM_MIN_VAL)
                              / (axis->max - axis->min)
                          + PWM_MIN_VAL + axis->origin_analog_val;

    controller_write_analog_in(axis->controller, analog_val);
}

void
axis_start_move (Axis *axis)
{
    if (axis->current == axis->target)
    {
        return;
    }

    // NOTE: Setting the state of the stage to moving before pulsing the
    // controller Trigger IN is important in ensuring accurate state tracking in
    // the unlikely circumstance that the stage's movement terminates before the
    // pulse is over, since `controller_pulse_trigger_in` is blocking.
    controller_set_stage_moving(axis->controller);
    controller_pulse_trigger_in(axis->controller);
}

bool
axis_is_moving (Axis *axis)
{
    return controller_is_stage_moving(axis->controller);
}

void
axis_move_end (Axis *axis)
{
    if (axis->current == axis->target)
    {
        return;
    }

    axis->current = axis->target;
}
