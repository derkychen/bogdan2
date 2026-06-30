#ifndef PLATFORM_SAMD21G18A_I2C_H
#define PLATFORM_SAMD21G18A_I2C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// TODO: Test fast frequency if standard works.
#define PLATFORM_SAMD21G18A_I2C_STANDARD_SCL_FREQUENCY_HZ (400000u)
#define PLATFORM_SAMD21G18A_I2C_STANDARD_SCL_RISE_NSEC    (300u)
#define PLATFORM_SAMD21G18A_I2C_FAST_SCL_FREQUENCY_HZ     (100000u)
#define PLATFORM_SAMD21G18A_I2C_FAST_SCL_RISE_NSEC        (1000u)

typedef enum
{
    PLATFORM_SAMD21G18A_I2C_RESULT_OK = 0,
    PLATFORM_SAMD21G18A_I2C_RESULT_ERROR,
    PLATFORM_SAMD21G18A_I2C_RESULT_INVALID_ARGUMENT,
    PLATFORM_SAMD21G18A_I2C_RESULT_NACK,
    PLATFORM_SAMD21G18A_I2C_RESULT_TIMEOUT,
} platform_samd21g18a_i2c_result_t;

/**
 * @brief I2C pin structure for storage of pin data.
 *
 * NOTE: This structure should only be used for initialization.
 */
typedef struct
{
    /** Pin port group. */
    uint8_t port_group;

    /** Pin number group. */
    uint8_t number;

    /** Pin peripheral function (C for PA16/17). */
    uint8_t peripheral_function;
} platform_samd21g18a_i2c_pin_t;

/** @brief I2C bus structure. */
typedef struct
{
    /** SDA I2C pin. */
    platform_samd21g18a_i2c_pin_t const *sda;

    /** SCL I2C pin. */
    platform_samd21g18a_i2c_pin_t const *scl;
} platform_samd21g18a_i2c_bus_t;

/** @brief Initialize the I2c peripheral. */
void platform_samd21g18a_i2c_init(void);

/**
 * @brief Configure one bus connected to the I2C.
 *
 * NOTE: The structure whose pointer is passed to this function should be
 *       initialized beforehand.
 */
void platform_samd21g18a_i2c_bus_configure(
    platform_samd21g18a_i2c_bus_t const *bus);

/** @brief Write bytes to an address. */
platform_samd21g18a_i2c_result_t platform_samd21g18a_i2c_write(
    uint8_t address, uint8_t const *data, size_t data_size);

/** @brief Read bytes from an address. */
platform_samd21g18a_i2c_result_t platform_samd21g18a_i2c_read(uint8_t  address,
                                                              uint8_t *data,
                                                              size_t data_size);

/**
 * @brief Write bytes to an address and then read data.
 *
 * This is useful for ADC, where a channel must be selected via an initial write
 * in order to obtain a reading.
 */
platform_samd21g18a_i2c_result_t platform_samd21g18a_i2c_write_read(
    uint8_t        address,
    uint8_t const *write_data,
    size_t         write_size,
    uint8_t       *read_data,
    size_t         read_size);

#endif
