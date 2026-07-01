#ifndef PLATFORM_SAMD21G18A_I2C_H
#define PLATFORM_SAMD21G18A_I2C_H

#include "platform/samd21g18a/pin.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// TODO: Test fast frequency if standard works.
#define PLATFORM_SAMD21G18A_I2C_STANDARD_SCL_FREQUENCY_HZ (100000u)
#define PLATFORM_SAMD21G18A_I2C_STANDARD_SCL_RISE_NSEC    (1000u)
#define PLATFORM_SAMD21G18A_I2C_FAST_SCL_FREQUENCY_HZ     (400000u)
#define PLATFORM_SAMD21G18A_I2C_FAST_SCL_RISE_NSEC        (300u)

/** @brief Status codes for I2C. */
typedef enum
{
    PLATFORM_SAMD21G18A_I2C_OK = 0,
    PLATFORM_SAMD21G18A_I2C_ERR,
    PLATFORM_SAMD21G18A_I2C_INVALID_ARGUMENT,
    PLATFORM_SAMD21G18A_I2C_NACK,
    PLATFORM_SAMD21G18A_I2C_TIMEOUT,
} platform_samd21g18a_i2c_status_code_t;

/** @brief I2C bus structure used for passing data at runtime. */
typedef struct
{
    /**
     * The SERCOM peripheral that is the I2C master.
     *
     * NOTE: The only supported SERCOM peripheral is SERCOM1. The API is general
     *       so that if support for other SERCOM peripherals can be implemented
     *       readily.
     */
    Sercom *sercom;
} platform_samd21g18a_i2c_bus_t;

/** @brief I2C configuration structure. */
typedef struct
{
    /** The SERCOM wrapper. */
    platform_samd21g18a_i2c_bus_t const *bus;

    /** SDA I2C pin. */
    platform_samd21g18a_pin_t const *sda;

    /** SCL I2C pin. */
    platform_samd21g18a_pin_t const *scl;

    /** SCL frequency in hertz. */
    uint32_t scl_frequency_hz;

    /** SCL rise time in nanoseconds. */
    uint32_t scl_rise_nsec;
} platform_samd21g18a_i2c_cfg_t;

/**
 * @brief Configure one bus connected to the I2C.
 *
 * There is no I2C initialization function because each bus is tied to a SERCOM
 * instance, whose initialization occurs in this function.
 *
 * NOTE: The structure whose pointer is passed to this function should be
 *       initialized beforehand.
 */
void platform_samd21g18a_i2c_configure(
    platform_samd21g18a_i2c_cfg_t const *cfg);

/** @brief Write bytes to an address. */
platform_samd21g18a_i2c_status_code_t platform_samd21g18a_i2c_write(
    platform_samd21g18a_i2c_bus_t const *bus,
    uint8_t                              address,
    uint8_t const                       *data,
    size_t                               data_size);

/** @brief Read bytes from an address. */
platform_samd21g18a_i2c_status_code_t platform_samd21g18a_i2c_read(
    platform_samd21g18a_i2c_bus_t const *bus,
    uint8_t                              address,
    uint8_t                             *data,
    size_t                               data_size);

/**
 * @brief Write bytes to an address and then read data.
 *
 * This is useful for ADC, where a channel must be selected via an initial write
 * in order to obtain a reading.
 */
platform_samd21g18a_i2c_status_code_t platform_samd21g18a_i2c_write_read(
    platform_samd21g18a_i2c_bus_t const *bus,
    uint8_t                              address,
    uint8_t const                       *write_data,
    size_t                               write_size,
    uint8_t                             *read_data,
    size_t                               read_size);

#endif
