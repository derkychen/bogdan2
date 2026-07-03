#ifndef DRIVERS_MCP4726_H
#define DRIVERS_MCP4726_H

#include "platform/samd21g18a/i2c.h"
#include <stdbool.h>
#include <stdint.h>

#define DRIVERS_MCP4726_MAX_VALUE (4095U)

/** @brief Status codes for MCP4726. */
typedef enum
{
    DRIVERS_MCP4726_STATUS_OK = 0,
    DRIVERS_MCP4726_STATUS_I2C_ERR,
} drivers_mcp4726_status_t;

/** @brief MCP4726 register address type. */
typedef uint8_t drivers_mcp4726_reg_t;

/** @brief MCP4726 device structure. */
typedef struct
{
    /** I2C master who controls the PCA9555. */
    platform_samd21g18a_i2c_master_t *master;

    /** Address of the MCP4726. */
    uint8_t address;
} drivers_mcp4726_device_t;

/** @brief Write a 12-bit digital value to the DAC. */
drivers_mcp4726_status_t mcp4726_write_output(
    drivers_mcp4726_device_t const *device, uint16_t value);

/**
 * @brief Write a 12-bit digital value to the DAC EEPROM.
 *
 * This function will likely not be used. The purpose of EEPROM is to store the
 * output even when the board is powered off. We have no use case for this.
 */
drivers_mcp4726_status_t mcp4726_write_output_ee(
    drivers_mcp4726_device_t const *device, uint16_t value);

#endif
