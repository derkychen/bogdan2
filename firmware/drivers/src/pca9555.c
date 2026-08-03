/**
 * @file pca9555.c
 * @brief Implementation of PCA9555 functionality.
 */
#include "drivers/pca9555.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/i2c.h"
#include <stddef.h>
#include <stdint.h>

#define REG_INPUT_PORT_0    (0x00u)
#define REG_OUTPUT_PORT_0   (0x02u)
#define REG_POLARITY_PORT_0 (0x04u)
#define REG_CFG_PORT_0      (0x06u)

static pca9555_status_t i2c_status_to_pca9555_status(i2c_status_t status);
static pca9555_status_t pca9555_write(pca9555_device_t const *device,
                                      pca9555_reg_t           reg,
                                      uint16_t                value);

pca9555_status_t
pca9555_write_outputs (pca9555_device_t const *device,
                       pca9555_outputs_t       outputs)
{
    ASSERT(device != NULL);

    return pca9555_write(device, REG_OUTPUT_PORT_0, outputs);
}

pca9555_status_t
pca9555_write_cfgs (pca9555_device_t const *device, pca9555_cfgs_t cfgs)
{
    ASSERT(device != NULL);

    return pca9555_write(device, REG_CFG_PORT_0, cfgs);
}

/** @brief Convert a I2C status code to a PCA9555 status code. */
static pca9555_status_t
i2c_status_to_pca9555_status (i2c_status_t status)
{
    switch (status)
    {
        case I2C_STATUS_OK:
            return PCA9555_STATUS_OK;
        case I2C_STATUS_ERR_BUS:
        case I2C_STATUS_ERR_NACK:
        case I2C_STATUS_ERR_TIMEOUT:
        default:
            return PCA9555_STATUS_ERR;
    }
}

/**
 * @brief Write 16 bits (each bit corresponding to a pin) to the PCA9555 over
 *        I2C.
 */
static pca9555_status_t
pca9555_write (pca9555_device_t const *device,
               pca9555_reg_t           reg,
               uint16_t                value)
{
    ASSERT(device != NULL);

    uint8_t data[3];

    data[0] = reg;
    data[1] = (uint8_t)(value & 0xFFu);
    data[2] = (uint8_t)((value >> 8u) & 0xFFu);

    i2c_status_t i2c_status
        = i2c_write(device->master, device->address, data, sizeof(data));

    return i2c_status_to_pca9555_status(i2c_status);
}
