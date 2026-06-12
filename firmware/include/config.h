/** @brief Hardware configuration information and initialization. */
#ifndef CONFIG_H
#define CONFIG_H

// WARNING: Do not change these values.
#define ADC_MIN_VAL           0u
#define ADC_MAX_VAL           4095u
#define ADC_TO_PWM_LEFT_SHIFT 4u
#define PWM_MIN_VAL           0u
#define PWM_MAX_VAL           65535u

// WARNING: Changing any of these values requires changing of wiring as well.
#define X_TRIGGER_IN_GPIO    18u
#define X_ANALOG_IN_GPIO     16u
#define X_TRIGGER_OUT_GPIO   20u
#define X_ANALOG_OUT_GPIO    26u
#define X_ANALOG_OUT_CHANNEL 0u

#define Y_TRIGGER_IN_GPIO    19u
#define Y_ANALOG_IN_GPIO     17u
#define Y_TRIGGER_OUT_GPIO   21u
#define Y_ANALOG_OUT_GPIO    27u
#define Y_ANALOG_OUT_CHANNEL 1u

#define PULSE_TRIGGER_GPIO 22u

/**
 * @brief Initialize hardware configuration.
 *
 * NOTE: This function only accounts for strictly hardware-based initialization,
 * (e.g. of pins, pull-up/down resistors, etc.). It does not account for
 * software initialization such as interrupts.
 */
void config_init(void);

#endif
