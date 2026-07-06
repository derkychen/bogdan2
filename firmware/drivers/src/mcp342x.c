#include "drivers/mcp342x.h"
#include "platform/samd21g18a/i2c.h"
#include <stddef.h>
#include <stdint.h>

#define CONFIG_RDY_START (0x80U)
#define CONFIG_RDY_BUSY  (0x80U)

#define CONFIG_CHANNEL_SHIFT    (5U)
#define CONFIG_ONESHOT_MODE     (0x00U)
#define CONFIG_RESOLUTION_SHIFT (2U)
#define CONFIG_GAIN_SHIFT       (0U)

/** @brief Convert a I2C status code to MCP342x status code. */
static drivers_mcp342x_status_t
i2c_status_to_mcp342x_status (platform_samd21g18a_i2c_status_t status)
{
    switch (status)
    {
        case PLATFORM_SAMD21G18A_I2C_STATUS_OK:
            return DRIVERS_MCP342X_STATUS_OK;
        case PLATFORM_SAMD21G18A_I2C_STATUS_NACK:
        case PLATFORM_SAMD21G18A_I2C_STATUS_TIMEOUT:
        case PLATFORM_SAMD21G18A_I2C_STATUS_ERR:
        default:
            return DRIVERS_MCP342X_STATUS_I2C_ERR;
    }
}

/** @brief Create the MCP342x configuration byte. */
static uint8_t
mcp342x_create_config_byte (drivers_mcp342x_channel_t    channel,
                            drivers_mcp342x_resolution_t resolution,
                            drivers_mcp342x_gain_t       gain)
{
    return (uint8_t)(CONFIG_RDY_START | ((uint8_t)channel << CONFIG_CHANNEL_SHIFT)
                     | CONFIG_ONESHOT_MODE
                     | ((uint8_t)resolution << CONFIG_RESOLUTION_SHIFT)
                     | ((uint8_t)gain << CONFIG_GAIN_SHIFT));
}

/** @brief Get number of data bytes returned by the ADC. */
static size_t
mcp342x_result_size (drivers_mcp342x_resolution_t resolution)
{
    if (resolution == DRIVERS_MCP342X_RESOLUTION_18_BIT)
    {
        return 3u;
    }
    else
    {
        return 2u;
    }
}

/** @brief Sign extend ADC result to int32_t. */
static int32_t
mcp342x_sign_extend (int32_t value, uint8_t bits)
{
    int32_t sign_bit = (int32_t)(1L << (bits - 1U));
    int32_t mask = (int32_t)((1L << bits) - 1L);

    value &= mask;

    if ((value & sign_bit) != 0)
    {
        value -= (int32_t)(1L << bits);
    }

    return value;
}

/** @brief Start one ADC conversion. */
drivers_mcp342x_status_t
mcp342x_start_conversion (drivers_mcp342x_device_t const *device,
                          drivers_mcp342x_channel_t       channel,
                          drivers_mcp342x_resolution_t    resolution,
                          drivers_mcp342x_gain_t          gain)
{
    uint8_t config_byte;

    PLATFORM_SAMD21G18A_ASSERT(device != NULL);

    config_byte = mcp342x_create_config_byte(channel, resolution, gain);
    
    return i2c_status_to_mcp342x_status(platform_samd21g18a_i2c_write(
        device->master, device->address, &config_byte, sizeof(config_byte)));
}

/** @brief Read the latest ADC conversion result. */
drivers_mcp342x_status_t
mcp342x_read (drivers_mcp342x_device_t const *device,
              drivers_mcp342x_resolution_t    resolution,
              int32_t                        *result)
{
    uint8_t data[4];
    size_t  result_size;
    uint_t  config;
    int32_t raw;
    platform_samd21g18a_i2c_status_t i2c_status;
    
    PLATFORM_SAMD21G18A_ASSERT(device != NULL);
    PLATFORM_SAMD21G18A_ASSERT(result != NULL);

    result_size = mcp342x_result_size(resolution);

    status = platform_samd21g18a_i2c_write_read(device->master, device->address, NULL, 0U, data, result_size + 1U);

    if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
    {
        return i2c_status_to_mcp342x_status(status);
    }

    config = data[result_size];

    if ((config & CONFIG_RDY_BUSY) != 0U)
    {
        return DRIVERS_MCP342X_STATUS_BUSY;
    }

    if (resolution == DRIVERS_MCP342X_RESOLUTION_18_BIT)
    {
        raw = ((int32_t)data[0] << 16U) | 
              ((int32_t)data[1] << 8U) | 
              (int32_t)data[2];
    }
    else
    {
        raw = ((int32_t)data[0] << 8) | 
              (int32_t)data[1];
        
        if (resolution == DRIVERS_MCP342X_RESOLUTION_12_BIT)
        {
            *result = mcp342x_sign_extend(raw, 12U);
        }
        else if (resolution == DRIVERS_MCP342X_RESOLUTION_14_BIT)
        {
            *result = mcp342x_sign_extend(raw, 14U);
        }
        else
        {
            *result = mcp342x_sign_extend(raw, 16U);
        }
    }

    return DRIVERS_MCP342X_STATUS_OK;
}