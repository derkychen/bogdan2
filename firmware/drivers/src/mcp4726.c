/**
 * @file mcp4726.c
 * @brief Implementation of MCP4726 functionality.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
#include "drivers/mcp4726.h"
#include "platform/samd21g18a/assert.h"
#include <stddef.h>
#include <stdint.h>

#define REG_DAC    (0x40u)
#define REG_DAC_EE (0x60u)

static mcp4726_status_t i2c_status_to_mcp4726_status(i2c_status_t status);
static mcp4726_status_t mcp4726_write(mcp4726_device_t const *device,
                                      mcp4726_reg_t           reg,
                                      uint16_t                value);

mcp4726_status_t
mcp4726_write_output (mcp4726_device_t const *device, uint16_t value)
{
    ASSERT(device != NULL);
    ASSERT(value <= MCP4726_MAX_VALUE);

    return mcp4726_write(device, REG_DAC, value);
}

mcp4726_status_t
mcp4726_write_output_ee (mcp4726_device_t const *device, uint16_t value)
{
    ASSERT(device != NULL);
    ASSERT(value <= MCP4726_MAX_VALUE);

    return mcp4726_write(device, REG_DAC_EE, value);
}

/** @brief Convert an I2C status code to a MCP4726 status code. */
static mcp4726_status_t
i2c_status_to_mcp4726_status (i2c_status_t status)
{
    switch (status)
    {
        case I2C_STATUS_OK:
            return MCP4726_STATUS_OK;
        case I2C_STATUS_ERR_BUS:
        case I2C_STATUS_ERR_NACK:
        case I2C_STATUS_ERR_TIMEOUT:
        default:
            return MCP4726_STATUS_ERR;
    }
}

/** @brief Write 16 bits (maximum 4095) to the MCP4726 over I2C. */
static mcp4726_status_t
mcp4726_write (mcp4726_device_t const *device,
               mcp4726_reg_t           reg,
               uint16_t                value)
{
    ASSERT(device != NULL);
    ASSERT(value <= MCP4726_MAX_VALUE);

    uint8_t data[3];

    data[0] = reg;
    data[1] = (uint8_t)(value >> 4u);
    data[2] = (uint8_t)((value & 0x0Fu) << 4u);

    return i2c_status_to_mcp4726_status(
        i2c_write(device->master, device->address, data, sizeof(data)));
}
