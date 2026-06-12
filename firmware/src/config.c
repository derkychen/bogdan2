#include "config.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

void
config_init (void)
{
    adc_init();

    // Configure Trigger IN for x-axis controller.
    gpio_init(X_TRIGGER_IN_GPIO);
    gpio_set_dir(X_TRIGGER_IN_GPIO, GPIO_OUT);
    gpio_put(X_TRIGGER_IN_GPIO, 0);

    // Configure Analog IN for x-axis controller.
    gpio_set_function(X_ANALOG_IN_GPIO, GPIO_FUNC_PWM);
    uint x_analog_in_slice = pwm_gpio_to_slice_num(X_ANALOG_IN_GPIO);
    pwm_set_wrap(x_analog_in_slice, 65535);
    pwm_set_enabled(x_analog_in_slice, true);

    // Configure Trigger OUT for x-axis controller.
    gpio_init(X_TRIGGER_OUT_GPIO);
    gpio_set_dir(X_TRIGGER_OUT_GPIO, GPIO_IN);
    gpio_pull_down(X_TRIGGER_OUT_GPIO);

    // Configure Analog OUT for x-axis controller.
    adc_gpio_init(X_ANALOG_OUT_GPIO);

    // Configure Trigger IN for y-axis controller.
    gpio_init(Y_TRIGGER_IN_GPIO);
    gpio_set_dir(Y_TRIGGER_IN_GPIO, GPIO_OUT);
    gpio_put(Y_TRIGGER_IN_GPIO, 0);

    // Configure Analog IN for x-axis controller.
    gpio_set_function(Y_ANALOG_IN_GPIO, GPIO_FUNC_PWM);
    uint y_analog_in_slice = pwm_gpio_to_slice_num(Y_ANALOG_IN_GPIO);
    pwm_set_wrap(x_analog_in_slice, 65535);
    pwm_set_enabled(x_analog_in_slice, true);

    // Configure Trigger OUT for x-axis controller.
    gpio_init(Y_TRIGGER_OUT_GPIO);
    gpio_set_dir(Y_TRIGGER_OUT_GPIO, GPIO_IN);
    gpio_pull_down(Y_TRIGGER_OUT_GPIO);
    
    // Configure Analog OUT for y-axis controller.
    adc_gpio_init(Y_ANALOG_OUT_GPIO);

    // Configure pulse counting input.
    gpio_init(PULSE_TRIGGER_GPIO);
    gpio_set_dir(PULSE_TRIGGER_GPIO, GPIO_IN);
    gpio_pull_down(PULSE_TRIGGER_GPIO);
}
