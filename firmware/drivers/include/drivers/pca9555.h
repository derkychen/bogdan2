#ifndef DRIVERS_PCA9555_H
#define DRIVERS_PCA9555_H

#include "platform/samd21g18a/i2c.h"
#include <stdint.h>

/** @brief Status codes for PCA9555. */
typedef enum
{
    DRIVERS_PCA9555_STATUS_OK = 0,
    DRIVERS_PCA9555_STATUS_ERR,
} drivers_pca9555_status_t;

/** @brief PCA9555 register address type. */
typedef uint8_t drivers_pca9555_reg_t;

/** @brief PCA9555 inputs type. */
typedef uint16_t drivers_pca9555_inputs_t;

/** @brief PCA9555 outputs type. */
typedef uint16_t drivers_pca9555_outputs_t;

/** @brief PCA9555 configurations type. */
typedef uint16_t drivers_pca9555_cfgs_t;

/** @brief PCA9555 polarities type. */
typedef uint16_t drivers_pca9555_polarities_t;

/** @brief PCA9555 device structure. */
typedef struct
{
    /** I2C master who controls the PCA9555. */
    platform_samd21g18a_i2c_master_t master;

    /** Address of the PCA9555. */
    platform_samd21g18a_i2c_slave_address_t address;
} drivers_pca9555_device_t;

/**
 * @brief Write all 16 output latch bits on the PCA9555 over I2C.
 *
 * Zero for LOW, one for HIGH.
 */
drivers_pca9555_status_t drivers_pca9555_write_outputs(
    drivers_pca9555_device_t const *device, drivers_pca9555_outputs_t outputs);

/**
 * @brief Write all 16 input configurations on the PCA9555 over I2C.
 *
 * Zero for output, one for input.
 */
drivers_pca9555_status_t drivers_pca9555_write_cfgs(
    drivers_pca9555_device_t const *device, drivers_pca9555_cfgs_t cfgs);

#endif
