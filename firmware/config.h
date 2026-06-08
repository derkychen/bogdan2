/**
 * @brief Interface between physical configuration of circuitry and code.
 */

#ifndef CONFIG_H
#define CONFIG_H

#define NUM_GPIO 30

#define X_TRIGGER_IN_GPIO 18
#define X_ANALOG_IN_GPIO 16
#define X_TRIGGER_OUT_GPIO 20

#define Y_TRIGGER_IN_GPIO 19
#define Y_ANALOG_IN_GPIO 17
#define Y_TRIGGER_OUT_GPIO 21

#define PULSE_TRIGGER_GPIO 22

/** @brief Initialize hardware configuration (i.e. pins, pull up/down, etc.). */
void config_init(void);

#endif
