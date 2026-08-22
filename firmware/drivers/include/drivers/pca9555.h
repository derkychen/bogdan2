/**
 * @file pca9555.h
 * @brief Driver for the PCA9555 chips on the IND.I/O baseboard.
 *
 * This module provides functionality for PCA9555 writing via I2C.
 *
 * @warning Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 *
 * @note For the purposes of this application, this driver is only used for
 *       configuring the range of the analog output.
 */
#ifndef DRIVERS_PCA9555_H
#define DRIVERS_PCA9555_H

#include "platform/samd21g18a/i2c.h"
#include <stdint.h>

/** @brief Status codes for PCA9555. */
typedef enum
{
    PCA9555_STATUS_OK = 0,
    PCA9555_STATUS_ERR,
} pca9555_status_t;

/** @brief PCA9555 register address type. */
typedef uint8_t pca9555_reg_t;

/** @brief PCA9555 inputs type. */
typedef uint16_t pca9555_inputs_t;

/** @brief PCA9555 outputs type. */
typedef uint16_t pca9555_outputs_t;

/** @brief PCA9555 configurations type. */
typedef uint16_t pca9555_cfgs_t;

/** @brief PCA9555 polarities type. */
typedef uint16_t pca9555_polarities_t;

/** @brief PCA9555 device structure. */
typedef struct
{
    i2c_master_t        master;  /**< I2C master who controlling the PCA9555. */
    i2c_slave_address_t address; /**< Address of the PCA9555. */
} pca9555_device_t;

/**
 * @brief Write all 16 output latch bits on the PCA9555 over I2C.
 *
 * Zero for LOW, one for HIGH.
 */
pca9555_status_t pca9555_write_outputs(pca9555_device_t const *device,
                                       pca9555_outputs_t       outputs);

/**
 * @brief Write all 16 input configurations on the PCA9555 over I2C.
 *
 * Zero for output, one for input.
 */
pca9555_status_t pca9555_write_cfgs(pca9555_device_t const *device,
                                    pca9555_cfgs_t          cfgs);

#endif
