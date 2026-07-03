#ifndef DRIVERS_MCP342X_H
#define DRIVERS_MCP342X_H

#include "platform/samd21g18a/i2c.h"
#include <stdint.h>

/** @brief Status codes for MCP342x */
typedef enum
{
    DRIVERS_MCP342X_STATUS_OK = 0,
    DRIVERS_MCP342X_STATUS_BUSY,
    DRIVERS_MCP342X_STATUS_I2C_ERR,
} drivers_mcp342x_status_t;

/** @brief MCP342X device structure. */
typedef struct
{
    /** I2C master used to control the MCP342x */
    platform_samd21g18a_i2c_master_t    *master; 

    /** Address of the MCP342x */
    uint8_t                             address;
} drivers_mcp342x_device_t;


/** @brief MCP342x input channels. */
typedef enum
{
    DRIVERS_MCP342X_CHANNEL_1 = 0u,
    DRIVERS_MCP342X_CHANNEL_2 = 1u,
    DRIVERS_MCP342X_CHANNEL_3 = 2u,
    DRIVERS_MCP342X_CHANNEL_4 = 3u,
} drivers_mcp342x_channel_t;

/** @brief MCP342x resolution settings. */
typedef enum
{
    DRIVERS_MCP342X_RESOLUTION_12_BIT = 0u,
    DRIVERS_MCP342X_RESOLUTION_14_BIT = 1u,
    DRIVERS_MCP342X_RESOLUTION_16_BIT = 2u,
    DRIVERS_MCP342X_RESOLUTION_18_BIT = 3u,
} drivers_mcp342x_resolution_t;

/** @brief MCP342x gain settings. */
typedef enum
{
    DRIVERS_MCP342X_GAIN_1X = 0u,
    DRIVERS_MCP342X_GAIN_2X = 1u,
    DRIVERS_MCP342X_GAIN_4X = 2u,
    DRIVERS_MCP342X_GAIN_8X = 3u,
} drivers_mcp342x_gain_t;

/** @brief Start one ADC conversion. */
drivers_mcp342x_status_t mcp342x_start_conversion(
    drivers_mcp342x_device_t const *device,
    drivers_mcp342x_channel_t       channel,
    drivers_mcp342x_resolution_t    resolution,
    drivers_mcp342x_gain_t          gain);

/** @brief Read the latest ADC conversion result. */
drivers_mcp342x_status_t mcp342x_read(
    drivers_mcp342x_device_t const *device,
    drivers_mcp342x_resolution_t    resolution,
    int32_t                        *result);

#endif


