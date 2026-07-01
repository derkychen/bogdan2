#include "drivers/pca9555.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/i2c.h"
#include <stddef.h>
#include <stdint.h>

#define PCA9555_REG_INPUT_PORT_0    (0x00u)
#define PCA9555_REG_OUTPUT_PORT_0   (0x02u)
#define PCA9555_REG_POLARITY_PORT_0 (0x04u)
#define PCA9555_REG_CFG_PORT_0      (0x06u)

/** @brief Convert a I2C status code to a PCA9555 status code. */
static drivers_pca9555_status_code_t
i2c_status_code_to_pca9555_status_code (
    platform_samd21g18a_i2c_status_code_t status_code)
{
    switch (status_code)
    {
        case PLATFORM_SAMD21G18A_I2C_OK:
            return DRIVERS_PCA9555_OK;

        case PLATFORM_SAMD21G18A_I2C_INVALID_ARGUMENT:
            return DRIVERS_PCA9555_INVALID_ARGUMENT;

        case PLATFORM_SAMD21G18A_I2C_NACK:
        case PLATFORM_SAMD21G18A_I2C_TIMEOUT:
        case PLATFORM_SAMD21G18A_I2C_ERR:
        default:
            return DRIVERS_PCA9555_I2C_ERR;
    }
}

/**
 * @brief Read 16 bits (each corresponding to a pin) from the PCA9555 over I2C.
 */
static drivers_pca9555_status_code_t
pca9555_read_u16 (drivers_pca9555_device_t const *device,
                  uint8_t                         reg,
                  uint16_t                       *value)
{
    platform_samd21g18a_i2c_status_code_t i2c_status;
    uint8_t                               data[2];

    PLATFORM_SAMD21G18A_ASSERT(device != NULL);
    PLATFORM_SAMD21G18A_ASSERT(value != NULL);

    if ((device == NULL) || (device->bus == NULL) || (value == NULL))
    {
        return DRIVERS_PCA9555_INVALID_ARGUMENT;
    }

    i2c_status = platform_samd21g18a_i2c_write_read(
        device->bus, device->address, &reg, 1u, data, sizeof(data));

    if (i2c_status != PLATFORM_SAMD21G18A_I2C_OK)
    {
        return i2c_status_code_to_pca9555_status_code(i2c_status);
    }

    *value = ((uint16_t)data[1] << 8u) | data[0];

    return DRIVERS_PCA9555_OK;
}

/**
 * @brief Write 16 bits (each bit corresponding to a pin) to the PCA9555 over
 *        I2C.
 */
static drivers_pca9555_status_code_t
pca9555_write_u16 (drivers_pca9555_device_t const *device,
                   uint8_t                         reg,
                   uint16_t                        value)
{
    platform_samd21g18a_i2c_status_code_t i2c_status;
    uint8_t                               data[3];

    PLATFORM_SAMD21G18A_ASSERT(device != NULL);

    if ((device == NULL) || (device->bus == NULL))
    {
        return DRIVERS_PCA9555_INVALID_ARGUMENT;
    }

    data[0] = reg;
    data[1] = (uint8_t)(value & 0xffu);
    data[2] = (uint8_t)((value >> 8u) & 0xffu);

    i2c_status = platform_samd21g18a_i2c_write(
        device->bus, device->address, data, sizeof(data));

    return i2c_status_code_to_pca9555_status_code(i2c_status);
}

drivers_pca9555_status_code_t
drivers_pca9555_read_inputs (drivers_pca9555_device_t const *device,
                             uint16_t                       *inputs)
{
    return pca9555_read_u16(device, PCA9555_REG_INPUT_PORT_0, inputs);
}

drivers_pca9555_status_code_t
drivers_pca9555_write_outputs (drivers_pca9555_device_t const *device,
                               uint16_t                        outputs)
{
    return pca9555_write_u16(device, PCA9555_REG_OUTPUT_PORT_0, outputs);
}

drivers_pca9555_status_code_t
drivers_pca9555_write_cfg (drivers_pca9555_device_t const *device, uint16_t cfg)
{
    return pca9555_write_u16(device, PCA9555_REG_CFG_PORT_0, cfg);
}

drivers_pca9555_status_code_t
drivers_pca9555_write_polarity (drivers_pca9555_device_t const *device,
                                uint16_t                        polarity)
{
    return pca9555_write_u16(device, PCA9555_REG_POLARITY_PORT_0, polarity);
}
