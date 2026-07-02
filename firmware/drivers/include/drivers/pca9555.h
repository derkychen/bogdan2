#ifndef DRIVERS_PCA9555_H
#define DRIVERS_PCA9555_H

#include "platform/samd21g18a/i2c.h"
#include <stdint.h>

/** @brief Status codes for PCA9555. */
typedef enum
{
    DRIVERS_PCA9555_STATUS_OK = 0,
    DRIVERS_PCA9555_STATUS_I2C_ERR,
} drivers_pca9555_status_t;

/** @brief PCA9555 device structure. */
typedef struct
{
    /** I2C master who controls the PCA9555. */
    platform_samd21g18a_i2c_master_t *master;

    /** Address of the PCA9555. */
    uint8_t address;
} drivers_pca9555_device_t;

/**
 * @brief Read all 16 inputs on the PCA9555 over I2C.
 *
 * Zero for LOW, one for HIGH.
 */
drivers_pca9555_status_t drivers_pca9555_read_inputs(
    drivers_pca9555_device_t const *device, uint16_t *inputs);

/**
 * @brief Write all 16 output latch bits on the PCA9555 over I2C.
 *
 * Zero for LOW, one for HIGH.
 */
drivers_pca9555_status_t drivers_pca9555_write_outputs(
    drivers_pca9555_device_t const *device, uint16_t outputs);

/**
 * @brief Write all 16 input configurations on the PCA9555 over I2C.
 *
 * Zero for output, one for input.
 */
drivers_pca9555_status_t drivers_pca9555_write_cfg(
    drivers_pca9555_device_t const *device, uint16_t cfg);

/**
 * @brief Write all 16 input polarities on the PCA9555 over I2C.
 *
 * Zero for normal, one for inversion.
 */
drivers_pca9555_status_t drivers_pca9555_write_polarity(
    drivers_pca9555_device_t const *device, uint16_t polarity);

#endif
