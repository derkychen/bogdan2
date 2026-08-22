/**
 * @file i2c.h
 * @brief I2C communication.
 *
 * This module provides functionality for I2C communication. It is aims to be as
 * generic as possible, though currently it only supports an I2C bus on PA16 and
 * PA17, which is the bus required to communicate with the IND.I/O baseboard
 * I/O.
 *
 * @warning Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
#ifndef PLATFORM_SAMD21G18A_I2C_H
#define PLATFORM_SAMD21G18A_I2C_H

#include "platform/samd21g18a/pin.h"
#include <stddef.h>
#include <stdint.h>

#define I2C_SCL_FREQUENCY_STANDARD_HZ (100000u)
#define I2C_SCL_RISE_STANDARD_NS      (1000u)
#define I2C_SCL_FREQUENCY_FAST_HZ     (400000u)
#define I2C_SCL_RISE_FAST_NS          (300u)

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

/** @brief I2C slave address type (seven-bit). */
typedef uint8_t i2c_slave_address_t;

/** @brief Type for I2C pins. */
typedef struct
{
    pin_t const     *pin; /**< Pin. */
    i2c_sercom_pad_t pad; /**< SERCOM pad. */
} i2c_pin_t;

/** @brief Initialize an I2C master. */
void i2c_init(i2c_master_t     master,
              i2c_pin_t const *sda,
              i2c_pin_t const *scl,
              uint32_t         scl_frequency_hz,
              uint32_t         scl_rise_ns);

/**
 * @brief Configure an I2C master.
 *
 * @note This function should only be called after `i2c_init` has been called on
 *       the same master.
 */
void i2c_configure(i2c_master_t master);

/** @brief Write bytes to an address. */
i2c_status_t i2c_write(i2c_master_t        master,
                       i2c_slave_address_t slave_address,
                       uint8_t const      *data,
                       size_t              data_size);

/** @brief Read bytes from an address. */
i2c_status_t i2c_read(i2c_master_t        master,
                      i2c_slave_address_t slave_address,
                      uint8_t            *data,
                      size_t              data_size);

/**
 * @brief Write bytes to an address and then read data.
 *
 * This is useful for ADC, where a channel must be selected via an initial
 * write in order to obtain a reading.
 */
i2c_status_t i2c_write_read(i2c_master_t        master,
                            i2c_slave_address_t slave_address,
                            uint8_t const      *write_data,
                            size_t              write_size,
                            uint8_t            *read_data,
                            size_t              read_size);

#endif
