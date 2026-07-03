#include "drivers/mcp4726.h"
#include "platform/samd21g18a/assert.h"
#include <stddef.h>
#include <stdint.h>

#define REG_DAC        (0x40U)
#define REG_DAC_EEPROM (0x60U)

/** @brief Convert a I2C status code to a MCP4726 status code. */
static drivers_mcp4726_status_t
i2c_status_to_mcp4726_status (platform_samd21g18a_i2c_status_t status)
{
    switch (status)
    {
        case PLATFORM_SAMD21G18A_I2C_STATUS_OK:
            return DRIVERS_MCP4726_STATUS_OK;
        case PLATFORM_SAMD21G18A_I2C_STATUS_NACK:
        case PLATFORM_SAMD21G18A_I2C_STATUS_TIMEOUT:
        case PLATFORM_SAMD21G18A_I2C_STATUS_ERR:
        default:
            return DRIVERS_MCP4726_STATUS_I2C_ERR;
    }
}

/** @brief Write 16 bits (maximum 4095) to the MCP4726 over I2C. */
static drivers_mcp4726_status_t
mcp4726_write (drivers_mcp4726_device_t const *device,
               drivers_mcp4726_reg_t           reg,
               uint16_t                        value)
{
    uint8_t data[3];

    PLATFORM_SAMD21G18A_ASSERT(device != NULL);
    PLATFORM_SAMD21G18A_ASSERT(value <= DRIVERS_MCP4726_MAX_VALUE);

    data[0] = reg;
    data[1] = (uint8_t)(value >> 4u);
    data[2] = (uint8_t)((value & 0x0fu) << 4u);

    return i2c_status_to_mcp4726_status(platform_samd21g18a_i2c_write(
        device->master, device->address, data, sizeof(data)));
}

drivers_mcp4726_status_t
mcp4726_write_output (drivers_mcp4726_device_t const *device, uint16_t value)
{
    return mcp4726_write(device, REG_DAC, value);
}

drivers_mcp4726_status_t
mcp4726_write_output_ee (drivers_mcp4726_device_t const *device, uint16_t value)
{
    return mcp4726_write(device, REG_DAC_EEPROM, value);
}
