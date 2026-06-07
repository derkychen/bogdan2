#ifndef AXIS_H
#define AXIS_H

#define ANALOG_MIN_VAL 0
#define ANALOG_MAX_VAL 65536

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

/**
 * @brief Initialize an axis.
 */
void *axis_init(Axis *axis, int min_unit, int max_unit, int unit_nm,
                int trigger_in, int analog_in, int trigger_out);

/**
 * @brief Number of points on the axis.
 */
int axis_num_points(Axis *axis);

/**
 * @brief Set the target of the stage.
 *
 * This function will only work as expected if the controller is in closed loop
 * mode. It sets the next target of the stage to the point corresponding to
 * a given coordinate, set through the controller Analog IN.
 */
void axis_set_target(Axis *axis, int coord);

/**
 * @brief Move the stage to its destination.
 *
 * This function will only work as expected if the controller is in closed loop
 * mode. It moves the stage to its next destination by sending a pulse to the
 * controller Trigger IN. A delay can be added to account for the time taken to
 * acquire post-trigger samples.
 */
void axis_start_move(Axis *axis);

#endif
