#include "axis.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include <stdbool.h>

void *axis_init(Axis *axis, int min, int max, int unit_nm, int trigger_in,
                int analog_in, int trigger_out) {
  axis->min = min;
  axis->max = max;
  axis->unit_nm = unit_nm;

  axis->trigger_in = trigger_in;
  axis->analog_in = analog_in;
  axis->trigger_out = trigger_out;

  return axis;
}

int axis_num_points(Axis *axis) { return axis->max - axis->min + 1; }

// TODO: Check if PWM jitter causes controller to think target is changing. Will
//       need DAC or low-pass filter maybe.
void axis_set_target(Axis *axis, int coord) {

  // Analog value of the coordinate
  int analog_val = (coord - axis->min) * (ANALOG_MAX_VAL - ANALOG_MIN_VAL) /
                       (axis->max - axis->min) +
                   ANALOG_MIN_VAL;

  // Write the analog value to the controller Analog IN using PWM
  int slice = pwm_gpio_to_slice_num(axis->analog_in);
  int channel = pwm_gpio_to_channel(axis->analog_in);

  pwm_set_chan_level(slice, channel, analog_val);
}

void axis_start_move(Axis *axis) {

  // Send a voltage pulse to the controller Trigger IN
  gpio_put(axis->trigger_in, 1);
  sleep_us(10);
  gpio_put(axis->trigger_in, 0);

  // Set state variable, this value will be changed upon an interrupt
  axis->moving = true;
}
