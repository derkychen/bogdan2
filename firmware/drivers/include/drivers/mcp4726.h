/**
 * @file mcp4726.h
 * @brief Driver for the MCP4726 chips on the IND.I/O baseboard.
 *
 * This module provides functionality for DAC conversions via I2C for the
 * MCP4726 chips, which are responsible for analog outputs.
 *
 * NOTE: If the range of analog outputs is not configured with using the PCA9555
 *       drivers, analog functionality from this module alone will not work.
 */
#ifndef DRIVERS_MCP4726_H
#define DRIVERS_MCP4726_H

#include "platform/samd21g18a/i2c.h"
#include <stdbool.h>
#include <stdint.h>

#define MCP4726_MAX_VALUE (0x0FFFu)

/** @brief Status codes for MCP4726. */
typedef enum
{
    MCP4726_STATUS_OK = 0,
    MCP4726_STATUS_ERR,
} mcp4726_status_t;

/** @brief MCP4726 register address type. */
typedef uint8_t mcp4726_reg_t;

/** @brief MCP4726 device structure. */
typedef struct
{
    i2c_master_t        master;  /**< I2C master controlling the MCP4726. */
    i2c_slave_address_t address; /**< Address of the MCP4726. */
} mcp4726_device_t;

/** @brief Write a 12-bit digital value to the MCP4726 device. */
mcp4726_status_t mcp4726_write_output(mcp4726_device_t const *device,
                                      uint16_t                value);

/**
 * @brief Write a 12-bit digital value to the MCP4726 device EEPROM.
 *
 * This function will likely not be used. The purpose of EEPROM is to store the
 * output even when the board is powered off. We have no use case for this.
 */
mcp4726_status_t mcp4726_write_output_ee(mcp4726_device_t const *device,
                                         uint16_t                value);

#endif
