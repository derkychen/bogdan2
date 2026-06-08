#ifndef AXIS_H
#define AXIS_H

#include <stdbool.h>

/**
 * @brief Interface between coordinate system and stage.
 *
 * The user of this structure must ensure that the pins are wired properly and
 * configured in the software.
 */
typedef struct Axis {

  /** Minimum coordinate on the axis in units. */
  int min;

  /** Maximum coordinate on the axis in units. */
  int max;

  /** The length of each unit on the axis in nanometres */
  int unit_nm;

  /** The digital pin that the controller Trigger IN is connected to. */
  int trigger_in;

  /** The analog pin that the controller Analog IN is connected to. */
  int analog_in;

  /** The digital pin that the controller Trigger OUT is connected to. */
  int trigger_out;

  /** State variable for current coordinate on the axis in units. */
  int cur;

  /** State variable for target coordinate on the axis in units. */
  int target;

  /** State variable for whether the stage is moving. */
  volatile bool moving;

} Axis;

/** @brief Status codes for initialization. */
typedef enum AxisInitStatusCode {
  AXIS_INIT_OK = 0,
  AXIS_INIT_ERR_NULL,
  AXIS_INIT_ERR_INVALID_GPIO,
  AXIS_INIT_ERR_UNSUPPORTED_GPIO,
  AXIS_INIT_ERR_DUPLICATE_TRIGGER_OUT,
} AxisInitStatusCode;

/** @brief Initialize an axis. */
AxisInitStatusCode axis_init(Axis *axis, int min, int max, int unit_nm,
                             int trigger_in, int analog_in, int trigger_out);

/** @brief Initialize IRQ configurations for both axes */
void axis_irq_init(void);

/** @brief Number of points on the axis. */
int axis_num_points(Axis *axis);

/**
 * @brief Set the target of the stage.
 *
 * This function will only work as expected if the controller is in closed loop
 * mode. It sets the next target of the stage to the point corresponding to
 * a given coordinate, set through the controller Analog IN.
 */
void axis_set_target(Axis *axis, int target);

/**
 * @brief Start the stage's movement to its target.
 *
 * This function will only work as expected if the controller is in closed loop
 * mode. It sends a pulse to the controller Trigger IN.
 */
void axis_start_move(Axis *axis);

/** @brief Update state variables when the stage is at its destination. */
void axis_move_end(Axis *axis);

#endif
