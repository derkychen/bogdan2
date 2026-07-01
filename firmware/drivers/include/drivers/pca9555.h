#ifndef DRIVERS_PCA9555_H
#define DRIVERS_PCA9555_H

#include "platform/samd21g18a/i2c.h"
#include <stdint.h>

/** @brief Status codes for PCA9555. */
typedef enum
{
    DRIVERS_PCA9555_OK = 0,
    DRIVERS_PCA9555_INVALID_ARGUMENT,
    DRIVERS_PCA9555_I2C_ERR,
} drivers_pca9555_status_code_t;

/** @brief PCA9555 device structure. */
typedef struct
{
    /** I2C bus whose master controls the PCA9555. */
    platform_samd21g18a_i2c_bus_t const *bus;

    /** Address of the PCA9555. */
    uint8_t address;
} drivers_pca9555_device_t;

/**
 * @brief Read all 16 inputs on the PCA9555 over I2C.
 *
 * One for HIGH and zero for LOW.
 */
drivers_pca9555_status_code_t drivers_pca9555_read_inputs(
    drivers_pca9555_device_t const *device, uint16_t *inputs);

/**
 * @brief Write all 16 output latch bits on the PCA9555 over I2C.
 *
 * One for HIGH and zero for LOW.
 */
drivers_pca9555_status_code_t drivers_pca9555_write_outputs(
    drivers_pca9555_device_t const *device, uint16_t outputs);

/**
 * @brief Write all 16 input configurations on the PCA9555 over I2C.
 *
 * One for input and zero for output.
 */
drivers_pca9555_status_code_t drivers_pca9555_write_cfg(
    drivers_pca9555_device_t const *device, uint16_t cfg);

/**
 * @brief Write all 16 input polarities on the PCA9555 over I2C.
 *
 * One for inversion and zero for normal polarity.
 */
drivers_pca9555_status_code_t drivers_pca9555_write_polarity(
    drivers_pca9555_device_t const *device, uint16_t polarity);

#endif
