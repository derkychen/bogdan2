#ifndef BOARD_INDIO_IO_CFG_H
#define BOARD_INDIO_IO_CFG_H

#include "board/indio/analog_output.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/i2c.h"

// TODO: Test interrupt and digital read capability on necessary pins.
// NOTE: This module only exposes the handles for pins used in this project.
/** @brief Handle for the IND.I/O expansion port pin D4/A6 as a digital pin. */
extern platform_samd21g18a_digital_pin_t const board_indio_expansion_d4_digital;

/** @brief Handle for the IND.I/O expansion port pin D5/PWM as a digital pin. */
extern platform_samd21g18a_digital_pin_t const board_indio_expansion_d5_digital;

/** @brief Handle for the IND.I/O expansion port pin D2/SDA as a digital pin. */
extern platform_samd21g18a_eic_pin_t const board_indio_expansion_d2_eic;

/** @brief Handle for the IND.I/O expansion port pin D7 as a digital pin. */
extern platform_samd21g18a_eic_pin_t const board_indio_expansion_d7_eic;

/** @brief Handle for the IND.I/O expansion port pin D6/A7 as a digital pin. */
extern platform_samd21g18a_eic_pin_t const board_indio_expansion_d6_eic;

/**
 * @brief Handle for the SAMD21G18A pin corresponding to the I2C SDA on the
 *        IND.I/O.
 */
extern platform_samd21g18a_i2c_pin_t const board_indio_bus_sda;

/**
 * @brief Handle for the SAMD21G18A pin corresponding to the I2C SCL on the
 *        IND.I/O.
 */
extern platform_samd21g18a_i2c_pin_t const board_indio_bus_scl;

/** @brief Handle for the IND.I/O analog output CH1. */
extern board_indio_analog_output_channel_t const board_indio_analog_output_ch1;

/** @brief Handle for the IND.I/O analog output CH2. */
extern board_indio_analog_output_channel_t const board_indio_analog_output_ch2;

#endif
