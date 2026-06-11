#include "config.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

void
config_init (void)
{
    // Trigger IN for x-axis controller
    gpio_init(X_TRIGGER_IN_GPIO);
    gpio_set_dir(X_TRIGGER_IN_GPIO, GPIO_OUT);
    gpio_put(X_TRIGGER_IN_GPIO, 0);

    // Analog IN for x-axis controller
    gpio_set_function(X_ANALOG_IN_GPIO, GPIO_FUNC_PWM);
    uint x_analog_in_slice = pwm_gpio_to_slice_num(X_ANALOG_IN_GPIO);
    pwm_set_wrap(x_analog_in_slice, 65535);
    pwm_set_enabled(x_analog_in_slice, true);

    // Trigger OUT for x-axis controller
    gpio_init(X_TRIGGER_OUT_GPIO);
    gpio_set_dir(X_TRIGGER_OUT_GPIO, GPIO_IN);
    gpio_pull_down(X_TRIGGER_OUT_GPIO);

    // Trigger IN for y-axis controller
    gpio_init(Y_TRIGGER_IN_GPIO);
    gpio_set_dir(Y_TRIGGER_IN_GPIO, GPIO_OUT);
    gpio_put(Y_TRIGGER_IN_GPIO, 0);

    // Analog IN for x-axis controller
    gpio_set_function(Y_ANALOG_IN_GPIO, GPIO_FUNC_PWM);
    uint y_analog_in_slice = pwm_gpio_to_slice_num(Y_ANALOG_IN_GPIO);
    pwm_set_wrap(x_analog_in_slice, 65535);
    pwm_set_enabled(x_analog_in_slice, true);

    // Trigger OUT for x-axis controller
    gpio_init(Y_TRIGGER_OUT_GPIO);
    gpio_set_dir(Y_TRIGGER_OUT_GPIO, GPIO_IN);
    gpio_pull_down(Y_TRIGGER_OUT_GPIO);

    // Pulse counting input
    gpio_init(PULSE_TRIGGER_GPIO);
    gpio_set_dir(PULSE_TRIGGER_GPIO, GPIO_IN);
    gpio_pull_down(PULSE_TRIGGER_GPIO);
}
