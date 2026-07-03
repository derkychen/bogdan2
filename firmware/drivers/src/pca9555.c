#include "drivers/pca9555.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/i2c.h"
#include <stddef.h>
#include <stdint.h>

#define REG_INPUT_PORT_0    (0x00U)
#define REG_OUTPUT_PORT_0   (0x02U)
#define REG_POLARITY_PORT_0 (0x04U)
#define REG_CFG_PORT_0      (0x06U)

/** @brief Convert a I2C status code to a PCA9555 status code. */
static drivers_pca9555_status_t
i2c_status_to_pca9555_status (platform_samd21g18a_i2c_status_t status)
{
    switch (status)
    {
        case PLATFORM_SAMD21G18A_I2C_STATUS_OK:
            return DRIVERS_PCA9555_STATUS_OK;
        case PLATFORM_SAMD21G18A_I2C_STATUS_NACK:
        case PLATFORM_SAMD21G18A_I2C_STATUS_TIMEOUT:
        case PLATFORM_SAMD21G18A_I2C_STATUS_ERR:
        default:
            return DRIVERS_PCA9555_STATUS_I2C_ERR;
    }
}

/**
 * @brief Read 16 bits (each corresponding to a pin) from the PCA9555 over I2C.
 */
static drivers_pca9555_status_t
pca9555_read (drivers_pca9555_device_t const *device,
              drivers_pca9555_reg_t           reg,
              uint16_t                       *value)
{
    platform_samd21g18a_i2c_status_t i2c_status;
    uint8_t                          data[2];

    PLATFORM_SAMD21G18A_ASSERT(device != NULL);
    PLATFORM_SAMD21G18A_ASSERT(device->master != NULL);
    PLATFORM_SAMD21G18A_ASSERT(value != NULL);

    i2c_status = platform_samd21g18a_i2c_write_read(
        device->master, device->address, &reg, 1U, data, sizeof(data));

    if (i2c_status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
    {
        return i2c_status_to_pca9555_status(i2c_status);
    }

    *value = ((uint16_t)data[1] << 8U) | data[0];

    return DRIVERS_PCA9555_STATUS_OK;
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
    platform_samd21g18a_i2c_status_t i2c_status;
    uint8_t                          data[3];

    PLATFORM_SAMD21G18A_ASSERT(device != NULL);
    PLATFORM_SAMD21G18A_ASSERT(device->master != NULL);

    data[0] = reg;
    data[1] = (uint8_t)(value & 0xFFU);
    data[2] = (uint8_t)((value >> 8U) & 0xFFU);

    i2c_status = platform_samd21g18a_i2c_write(
        device->master, device->address, data, sizeof(data));

    return i2c_status_to_pca9555_status(i2c_status);
}

drivers_pca9555_status_t
drivers_pca9555_read_inputs (drivers_pca9555_device_t const *device,
                             drivers_pca9555_inputs_t       *inputs)
{
    return pca9555_read(device, REG_INPUT_PORT_0, inputs);
}

drivers_pca9555_status_t
drivers_pca9555_write_outputs (drivers_pca9555_device_t const *device,
                               drivers_pca9555_outputs_t       outputs)
{
    return pca9555_write(device, REG_OUTPUT_PORT_0, outputs);
}

drivers_pca9555_status_t
drivers_pca9555_write_cfgs (drivers_pca9555_device_t const *device,
                            drivers_pca9555_cfgs_t          cfgs)
{
    return pca9555_write(device, REG_CFG_PORT_0, cfgs);
}

drivers_pca9555_status_t
drivers_pca9555_write_polarities (drivers_pca9555_device_t const *device,
                                  drivers_pca9555_polarities_t    polarities)
{
    return pca9555_write(device, REG_POLARITY_PORT_0, polarities);
}
