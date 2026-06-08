#include "config.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <pico/types.h>
#include <stdbool.h>

void config_init() {
  // Controller I/O for x-axis
  gpio_init(X_TRIGGER_IN_GPIO);
  gpio_set_dir(X_TRIGGER_IN_GPIO, GPIO_OUT);
  gpio_put(X_TRIGGER_IN_GPIO, 0);

  gpio_set_function(X_ANALOG_IN_GPIO, GPIO_FUNC_PWM);
  uint x_analog_in_slice = pwm_gpio_to_slice_num(X_ANALOG_IN_GPIO);
  pwm_set_wrap(x_analog_in_slice, 65535);
  pwm_set_enabled(x_analog_in_slice, true);

  gpio_init(X_TRIGGER_OUT_GPIO);
  gpio_set_dir(X_TRIGGER_OUT_GPIO, GPIO_IN);
  gpio_pull_down(X_TRIGGER_OUT_GPIO);

  // Controller I/O for x-axis
  gpio_init(Y_TRIGGER_IN_GPIO);
  gpio_set_dir(Y_TRIGGER_IN_GPIO, GPIO_OUT);
  gpio_put(Y_TRIGGER_IN_GPIO, 0);

  gpio_set_function(Y_ANALOG_IN_GPIO, GPIO_FUNC_PWM);
  uint y_analog_in_slice = pwm_gpio_to_slice_num(Y_ANALOG_IN_GPIO);
  pwm_set_wrap(x_analog_in_slice, 65535);
  pwm_set_enabled(x_analog_in_slice, true);

  gpio_init(Y_TRIGGER_OUT_GPIO);
  gpio_set_dir(Y_TRIGGER_OUT_GPIO, GPIO_IN);
  gpio_pull_down(Y_TRIGGER_OUT_GPIO);
}
