#ifndef DRIVERS_MCP3424_H
#define DRIVERS_MCP3424_H

#include "platform/samd21g18a/i2c.h"

/** @brief Status codes for MCP3424. */
typedef enum
{
    DRIVERS_MCP3424_STATUS_OK = 0,
    DRIVERS_MCP3424_STATUS_BUSY,
    DRIVERS_MCP3424_STATUS_I2C_ERR,
} drivers_mcp3424_status_t;

/** @brief MCP3424 device structure. */
typedef struct
{
    /** I2C master used to control the MCP342x. */
    platform_samd21g18a_i2c_master_t *master;

    /** Address of the MCP3424. */
    platform_samd21g18a_i2c_slave_address_t address;
} drivers_mcp3424_device_t;

/** @brief MCP3424 input channels. */
typedef enum
{
    DRIVERS_MCP3424_CHANNEL_1 = 0U,
    DRIVERS_MCP3424_CHANNEL_2 = 1U,
    DRIVERS_MCP3424_CHANNEL_3 = 2U,
    DRIVERS_MCP3424_CHANNEL_4 = 3U,
} drivers_mcp3424_channel_t;

/** @brief MCP342x resolution settings. */
typedef enum
{
    DRIVERS_MCP3424_RESOLUTION_12_BIT = 0U,
    DRIVERS_MCP3424_RESOLUTION_14_BIT = 1U,
    DRIVERS_MCP3424_RESOLUTION_16_BIT = 2U,
    DRIVERS_MCP3424_RESOLUTION_18_BIT = 3U,
} drivers_mcp3424_resolution_t;

/** @brief MCP342x gain settings. */
typedef enum
{
    DRIVERS_MCP3424_GAIN_1X = 0U,
    DRIVERS_MCP3424_GAIN_2X = 1U,
    DRIVERS_MCP3424_GAIN_4X = 2U,
    DRIVERS_MCP3424_GAIN_8X = 3U,
} drivers_mcp3424_gain_t;

/** @brief Start one ADC conversion. */
drivers_mcp3424_status_t drivers_mcp3424_start_conversion(
    drivers_mcp3424_device_t const *device,
    drivers_mcp3424_channel_t       channel,
    drivers_mcp3424_resolution_t    resolution,
    drivers_mcp3424_gain_t          gain);

/** @brief Read the latest ADC conversion result. */
drivers_mcp3424_status_t drivers_mcp3424_read(
    drivers_mcp3424_device_t const *device,
    drivers_mcp3424_resolution_t    resolution,
    int32_t                        *result);

#endif
