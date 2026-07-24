#include "drivers/pca9555.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/i2c.h"
#include <stddef.h>
#include <stdint.h>

#define REG_INPUT_PORT_0    (0x00u)
#define REG_OUTPUT_PORT_0   (0x02u)
#define REG_POLARITY_PORT_0 (0x04u)
#define REG_CFG_PORT_0      (0x06u)

static drivers_pca9555_status_t i2c_status_to_pca9555_status(
    platform_samd21g18a_i2c_status_t status);

static drivers_pca9555_status_t pca9555_write(
    drivers_pca9555_device_t const *device,
    drivers_pca9555_reg_t           reg,
    uint16_t                        value);

drivers_pca9555_status_t
drivers_pca9555_write_outputs (drivers_pca9555_device_t const *device,
                               drivers_pca9555_outputs_t       outputs)
{
    PLATFORM_SAMD21G18A_ASSERT(device != NULL);

    return pca9555_write(device, REG_OUTPUT_PORT_0, outputs);
}

drivers_pca9555_status_t
drivers_pca9555_write_cfgs (drivers_pca9555_device_t const *device,
                            drivers_pca9555_cfgs_t          cfgs)
{
    PLATFORM_SAMD21G18A_ASSERT(device != NULL);

    return pca9555_write(device, REG_CFG_PORT_0, cfgs);
}

/** @brief Convert a I2C status code to a PCA9555 status code. */
static drivers_pca9555_status_t
i2c_status_to_pca9555_status (platform_samd21g18a_i2c_status_t status)
{
    switch (status)
    {
        case PLATFORM_SAMD21G18A_I2C_STATUS_OK:
            return DRIVERS_PCA9555_STATUS_OK;
        case PLATFORM_SAMD21G18A_I2C_STATUS_ERR_BUS:
        case PLATFORM_SAMD21G18A_I2C_STATUS_ERR_NACK:
        case PLATFORM_SAMD21G18A_I2C_STATUS_ERR_TIMEOUT:
        default:
            return DRIVERS_PCA9555_STATUS_ERR;
    }
}

/**
 * @brief Write 16 bits (each bit corresponding to a pin) to the PCA9555 over
 *        I2C.
 */
static drivers_pca9555_status_t
pca9555_write (drivers_pca9555_device_t const *device,
               drivers_pca9555_reg_t           reg,
               uint16_t                        value)
{
    PLATFORM_SAMD21G18A_ASSERT(device != NULL);

    uint8_t data[3];

    data[0] = reg;
    data[1] = (uint8_t)(value & 0xFFu);
    data[2] = (uint8_t)((value >> 8u) & 0xFFu);

    platform_samd21g18a_i2c_status_t i2c_status = platform_samd21g18a_i2c_write(
        device->master, device->address, data, sizeof(data));

    return i2c_status_to_pca9555_status(i2c_status);
}
