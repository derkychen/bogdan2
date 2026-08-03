#ifndef PLATFORM_SAMD21G18A_I2C_H
#define PLATFORM_SAMD21G18A_I2C_H

#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define I2C_SCL_FREQUENCY_STANDARD_HZ (100000u)
#define I2C_SCL_RISE_STANDARD_NSEC    (1000u)
#define I2C_SCL_FREQUENCY_FAST_HZ     (400000u)
#define I2C_SCL_RISE_FAST_NSEC        (300u)

/** @brief Status codes for I2C. */
typedef enum
{
    I2C_STATUS_OK = 0,
    I2C_STATUS_ERR_BUS,
    I2C_STATUS_ERR_NACK,
    I2C_STATUS_ERR_TIMEOUT,
} i2c_status_t;

/** @brief Enumerate I2C masters, which correspond to SERCOM instances. */
typedef enum
{
    I2C_MASTER_SERCOM0 = 0,
    I2C_MASTER_SERCOM1,
    I2C_MASTER_SERCOM2,
    I2C_MASTER_SERCOM3,
    I2C_MASTER_SERCOM4,
    I2C_MASTER_SERCOM5,

    I2C_SERCOM_COUNT,
} i2c_master_t;

/** @brief Enumerate SERCOM pads. */
typedef enum
{
    I2C_SERCOM_PAD0 = 0,
    I2C_SERCOM_PAD1,
    I2C_SERCOM_PAD2,
    I2C_SERCOM_PAD3,
} i2c_sercom_pad_t;

/** @brief I2C slave address type. */
typedef uint8_t i2c_slave_address_t;

/** @brief Type for I2C pins. */
typedef struct
{
    /** Pin. */
    pin_t const *pin;

    /** SERCOM pad. */
    i2c_sercom_pad_t pad;
} i2c_pin_t;

/** @brief I2C configuration structure. */
typedef struct
{
} i2c_cfg_t;

/**
 * @brief Configure one bus connected to the I2C.
 *
 * There is no I2C initialization function because each bus is tied to a
 * SERCOM instance, whose initialization occurs in this function.
 *
 * NOTE: The structure whose pointer is passed to this function should be
 *       initialized beforehand.
 */
void i2c_configure(i2c_master_t     master,
                   i2c_pin_t const *sda,
                   i2c_pin_t const *scl,
                   uint32_t         scl_frequency_hz,
                   uint32_t         scl_rise_nsec);

/** @brief Write bytes to an address. */
i2c_status_t i2c_write(i2c_master_t   master,
                       uint8_t        address,
                       uint8_t const *data,
                       size_t         data_size);

/** @brief Read bytes from an address. */
i2c_status_t i2c_read(i2c_master_t master,
                      uint8_t      address,
                      uint8_t     *data,
                      size_t       data_size);

/**
 * @brief Write bytes to an address and then read data.
 *
 * This is useful for ADC, where a channel must be selected via an initial
 * write in order to obtain a reading.
 */
i2c_status_t i2c_write_read(i2c_master_t   master,
                            uint8_t        address,
                            uint8_t const *write_data,
                            size_t         write_size,
                            uint8_t       *read_data,
                            size_t         read_size);

#endif
