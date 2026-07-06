#include "drivers/mcp3424.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/i2c.h"
#include <stddef.h>
#include <stdint.h>

#define CFG_READY_START (0x80U)
#define CFG_READY_BUSY  (0x80U)

#define CFG_CHANNEL_SHIFT    (5U)
#define CFG_ONESHOT_MODE     (0x00U)
#define CFG_RESOLUTION_SHIFT (2U)
#define CFG_GAIN_SHIFT       (0U)

/** @brief Convert a I2C status code to MCP3424 status code. */
static drivers_mcp3424_status_t
i2c_status_to_mcp342x_status (platform_samd21g18a_i2c_status_t status)
{
    switch (status)
    {
        case PLATFORM_SAMD21G18A_I2C_STATUS_OK:
            return DRIVERS_MCP3424_STATUS_OK;
        case PLATFORM_SAMD21G18A_I2C_STATUS_NACK:
        case PLATFORM_SAMD21G18A_I2C_STATUS_TIMEOUT:
        case PLATFORM_SAMD21G18A_I2C_STATUS_ERR:
        default:
            return DRIVERS_MCP3424_STATUS_I2C_ERR;
    }
}

/** @brief Create a MCP342x configuration byte. */
static uint8_t
create_cfg_byte (drivers_mcp3424_channel_t    channel,
                 drivers_mcp3424_resolution_t resolution,
                 drivers_mcp3424_gain_t       gain)
{
    return (uint8_t)(CFG_READY_START | ((uint8_t)channel << CFG_CHANNEL_SHIFT)
                     | CFG_ONESHOT_MODE
                     | ((uint8_t)resolution << CFG_RESOLUTION_SHIFT)
                     | ((uint8_t)gain << CFG_GAIN_SHIFT));
}

/** @brief Sign extend an ADC result to `int32_t`. */
static int32_t
sign_extend (int32_t value, uint8_t bits)
{
    int32_t sign_bit = (int32_t)(1L << (bits - 1U));
    int32_t mask     = (int32_t)((1L << bits) - 1L);

    value &= mask;

    if ((value & sign_bit) != 0)
    {
        value -= (int32_t)(1L << bits);
    }

    return value;
}

/** @brief Start one ADC conversion. */
drivers_mcp3424_status_t
drivers_mcp342x_start_conversion (drivers_mcp3424_device_t const *device,
                                  drivers_mcp3424_channel_t       channel,
                                  drivers_mcp3424_resolution_t    resolution,
                                  drivers_mcp3424_gain_t          gain)
{
    uint8_t config_byte;

    PLATFORM_SAMD21G18A_ASSERT(device != NULL);

    config_byte = create_cfg_byte(channel, resolution, gain);

    return i2c_status_to_mcp342x_status(platform_samd21g18a_i2c_write(
        device->master, device->address, &config_byte, sizeof(config_byte)));
}

/** @brief Read the latest ADC conversion result. */
drivers_mcp3424_status_t
mcp342x_read (drivers_mcp3424_device_t const *device,
              drivers_mcp3424_resolution_t    resolution,
              int32_t                        *result)
{
    uint8_t                          data[4];
    size_t                           result_size;
    uint8_t                          config;
    int32_t                          raw;
    platform_samd21g18a_i2c_status_t status;

    PLATFORM_SAMD21G18A_ASSERT(device != NULL);
    PLATFORM_SAMD21G18A_ASSERT(result != NULL);

    result_size = (resolution == DRIVERS_MCP3424_RESOLUTION_18_BIT) ? 3u : 2u;

    status = platform_samd21g18a_i2c_write_read(
        device->master, device->address, NULL, 0U, data, result_size + 1U);

    if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
    {
        return i2c_status_to_mcp342x_status(status);
    }

    config = data[result_size];

    if ((config & CFG_READY_BUSY) != 0U)
    {
        return DRIVERS_MCP3424_STATUS_BUSY;
    }

    if (resolution == DRIVERS_MCP3424_RESOLUTION_18_BIT)
    {
        raw = ((int32_t)data[0] << 16U) | ((int32_t)data[1] << 8U)
              | (int32_t)data[2];
    }
    else
    {
        raw = ((int32_t)data[0] << 8) | (int32_t)data[1];

        if (resolution == DRIVERS_MCP3424_RESOLUTION_12_BIT)
        {
            *result = sign_extend(raw, 12U);
        }
        else if (resolution == DRIVERS_MCP3424_RESOLUTION_14_BIT)
        {
            *result = sign_extend(raw, 14U);
        }
        else
        {
            *result = sign_extend(raw, 16U);
        }
    }

    return DRIVERS_MCP3424_STATUS_OK;
}
