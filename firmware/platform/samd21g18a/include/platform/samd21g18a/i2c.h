#ifndef PLATFORM_SAMD21G18A_I2C_H
#define PLATFORM_SAMD21G18A_I2C_H

#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// TODO: Test fast frequency.
#define PLATFORM_SAMD21G18A_I2C_SCL_FREQUENCY_STANDARD_HZ (100000U)
#define PLATFORM_SAMD21G18A_I2C_SCL_RISE_STANDARD_NSEC    (1000U)
#define PLATFORM_SAMD21G18A_I2C_SCL_FREQUENCY_FAST_HZ     (400000U)
#define PLATFORM_SAMD21G18A_I2C_SCL_RISE_FAST_NSEC        (300U)

/** @brief Enumerate I2C masters, which correspond to SERCOM instances. */
typedef enum
{
    PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM0 = 0U,
    PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM1 = 1U,
    PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM2 = 2U,
    PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM3 = 3U,
    PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM4 = 4U,
    PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM5 = 5U,

    PLATFORM_SAMD21G18A_I2C_SERCOM_COUNT,
} platform_samd21g18a_i2c_master_t;

/** @brief Enumerate SERCOM pads. */
typedef enum
{
    PLATFORM_SAMD21G18A_I2C_SERCOM_PAD0 = 0U,
    PLATFORM_SAMD21G18A_I2C_SERCOM_PAD1 = 1U,
    PLATFORM_SAMD21G18A_I2C_SERCOM_PAD2 = 2U,
    PLATFORM_SAMD21G18A_I2C_SERCOM_PAD3 = 3U,
} platform_samd21g18a_i2c_sercom_pad_t;

/** @brief Status codes for I2C. */
typedef enum
{
    PLATFORM_SAMD21G18A_I2C_STATUS_OK = 0,
    PLATFORM_SAMD21G18A_I2C_STATUS_ERR_BUS,
    PLATFORM_SAMD21G18A_I2C_STATUS_ERR_NACK,
    PLATFORM_SAMD21G18A_I2C_STATUS_ERR_TIMEOUT,
} platform_samd21g18a_i2c_status_t;

/** @brief I2C slave address type. */
typedef uint8_t platform_samd21g18a_i2c_slave_address_t;

/** @brief Type for I2C pins. */
typedef struct
{
    /** Pin. */
    platform_samd21g18a_pin_t const *pin;

    /** SERCOM pad. */
    platform_samd21g18a_i2c_sercom_pad_t pad;
} platform_samd21g18a_i2c_pin_t;

/** @brief I2C configuration structure. */
typedef struct
{
    /** The I2C master. */
    platform_samd21g18a_i2c_master_t master;

    /** SDA I2C pin. */
    platform_samd21g18a_i2c_pin_t const *sda;

    /** SCL I2C pin. */
    platform_samd21g18a_i2c_pin_t const *scl;

    /** SCL frequency in hertz. */
    uint32_t scl_frequency_hz;

    /** SCL rise time in nanoseconds. */
    uint32_t scl_rise_nsec;
} platform_samd21g18a_i2c_cfg_t;

/**
 * @brief Configure one bus connected to the I2C.
 *
 * There is no I2C initialization function because each bus is tied to a
 * SERCOM instance, whose initialization occurs in this function.
 *
 * NOTE: The structure whose pointer is passed to this function should be
 *       initialized beforehand.
 */
void platform_samd21g18a_i2c_configure(
    platform_samd21g18a_i2c_cfg_t const *cfg);

/** @brief Write bytes to an address. */
platform_samd21g18a_i2c_status_t platform_samd21g18a_i2c_write(
    platform_samd21g18a_i2c_master_t master,
    uint8_t                          address,
    uint8_t const                   *data,
    size_t                           data_size);

/** @brief Read bytes from an address. */
platform_samd21g18a_i2c_status_t platform_samd21g18a_i2c_read(
    platform_samd21g18a_i2c_master_t master,
    uint8_t                          address,
    uint8_t                         *data,
    size_t                           data_size);

/**
 * @brief Write bytes to an address and then read data.
 *
 * This is useful for ADC, where a channel must be selected via an initial
 * write in order to obtain a reading.
 */
platform_samd21g18a_i2c_status_t platform_samd21g18a_i2c_write_read(
    platform_samd21g18a_i2c_master_t master,
    uint8_t                          address,
    uint8_t const                   *write_data,
    size_t                           write_size,
    uint8_t                         *read_data,
    size_t                           read_size);

#endif
