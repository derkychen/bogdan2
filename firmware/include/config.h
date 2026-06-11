#ifndef CONFIG_H
#define CONFIG_H

// WARNING: Changing any of these values requires changing of wiring as well.
#define X_TRIGGER_IN_GPIO  18
#define X_ANALOG_IN_GPIO   16
#define X_TRIGGER_OUT_GPIO 20

#define Y_TRIGGER_IN_GPIO  19
#define Y_ANALOG_IN_GPIO   17
#define Y_TRIGGER_OUT_GPIO 21

#define PULSE_TRIGGER_GPIO 22

/** @brief Initialize hardware configuration.
 *
 * NOTE: This function only accounts for strictly hardware-based initialization,
 * (e.g. of pins, pull-up/down resistors, etc.). It does not account for
 * software initialization such as interrupts.
 */
void config_init(void);

#endif
