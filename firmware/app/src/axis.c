/**
 * @file axis.c
 * @brief Implementation of coordinate system abstraction.
 *
 * @note If a different IND.I/O is used, The calibration done here should be
 *       redone. If it is found that the two analog output channels differ
 *       substantially, this file will need to be refactored to handle each
 *       channel separately.
 */
#include "app/axis.h"
#include "app/controller.h"
#include "platform/samd21g18a/assert.h"

#define MIN_MIN     (-1000)
#define MAX_MAX     (1000)
#define UNIT_MIN_NM (2000u)
#define UNIT_MAX_NM (1000000u)

// TODO: Extend stage range to +/- 6 mm if Thorlabs fixes the gain and offset
//       bug. Analog IN gain and offset currently restrict stage range to these
//       values.
#define STAGE_RANGE_MIN_NM (-3000000)
#define STAGE_RANGE_MAX_NM (3000000)

// NOTE: These are calibrated constants from voltmeter readings, defining the
//       lower nonlinear region and the main linear region. Since this main
//       linear region includes 10 volts, the upper nonlinear region (which
//       starts at around 10.5 volts) is unaccounted for.
#define LINEAR_LOW_ANALOG_VALUE  (256u)
#define LINEAR_LOW_VOLTAGE_MV    (590u)
#define LINEAR_HIGH_ANALOG_VALUE (3584u)
#define LINEAR_HIGH_VOLTAGE_MV   (9800u)

static uint16_t voltage_mv_to_analog_value(uint32_t voltage_mv);

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

    if (unit_nm < UNIT_MIN_NM)
    {
        return AXIS_STATUS_ERR_UNIT_TOO_SMALL;
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

    if (min_nm < STAGE_RANGE_MIN_NM || min_nm > STAGE_RANGE_MAX_NM
        || max_nm < STAGE_RANGE_MIN_NM || max_nm > STAGE_RANGE_MAX_NM)
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

    uint64_t position_nm = (uint64_t)(target_nm - STAGE_RANGE_MIN_NM);
    uint64_t range_nm    = (uint64_t)(STAGE_RANGE_MAX_NM - STAGE_RANGE_MIN_NM);
    uint32_t target_mv
        = (uint32_t)((position_nm * 10000u + range_nm / 2u) / range_nm);

    if (controller_write_analog_in(axis->controller,
                                   voltage_mv_to_analog_value(target_mv))
        != CONTROLLER_STATUS_OK)
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

    if (controller_should_move(axis->controller))
    {
        controller_set_stage_moving(axis->controller, true);
        controller_interrupt_enable(axis->controller);
        controller_pulse_trigger_in(axis->controller);
    }

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

void
axis_move_abort (axis_t *axis)
{
    ASSERT(axis != NULL);
    ASSERT(axis->controller != NULL);

    controller_set_stage_moving(axis->controller, false);
    controller_interrupt_disable(axis->controller);
    controller_invalidate_current(axis->controller);

    return;
}

/**
 * @brief Convert a desired voltage in millivolts to an analog value.
 *
 * This is done according to empirical data gathered for both analog channels.
 */
static uint16_t
voltage_mv_to_analog_value (uint32_t voltage_mv)
{
    ASSERT(voltage_mv <= 10000u);

    if (voltage_mv == 0u)
    {
        return 0u;
    }

    // Low voltage region (0 to 590 mV).
    if (voltage_mv <= LINEAR_LOW_VOLTAGE_MV)
    {
        return (uint16_t)(((uint64_t)voltage_mv * LINEAR_LOW_ANALOG_VALUE
                           + (LINEAR_LOW_VOLTAGE_MV / 2u))
                          / LINEAR_LOW_VOLTAGE_MV);
    }

    // Main, linear region (590 to 10000 mV),
    uint64_t numerator = (uint64_t)(voltage_mv - LINEAR_LOW_VOLTAGE_MV)
                         * (LINEAR_HIGH_ANALOG_VALUE - LINEAR_LOW_ANALOG_VALUE);

    uint32_t analog_value
        = LINEAR_LOW_ANALOG_VALUE
          + (uint32_t)((numerator
                        + ((LINEAR_HIGH_VOLTAGE_MV - LINEAR_LOW_VOLTAGE_MV)
                           / 2u))
                       / (LINEAR_HIGH_VOLTAGE_MV - LINEAR_LOW_VOLTAGE_MV));

    return (uint16_t)analog_value;
}
