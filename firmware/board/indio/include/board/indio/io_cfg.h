#ifndef BOARD_INDIO_IO_CFG_H
#define BOARD_INDIO_IO_CFG_H

#include "board/indio/analog_input.h"
#include "board/indio/analog_output.h"
#include "platform/samd21g18a/pin.h"

// TODO: Test all expansion port pins.

/** @brief Handle for the IND.I/O expansion port pin D0/RX. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d0;

/**
 * @brief Handle for the IND.I/O expansion port pin D1/TX.
 *
 * WARNING: The current module for MCU digital I/O does not support this pin.
 */
extern platform_samd21g18a_pin_t const board_indio_expansion_d1;

/** @brief Handle for the IND.I/O expansion port pin D2/SDA. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d2;

/** @brief Handle for the IND.I/O expansion port pin D3/SCL. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d3;

/** @brief Handle for the IND.I/O expansion port pin D4/A6. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d4;

/** @brief Handle for the IND.I/O expansion port pin D5/PWM. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d5;

/** @brief Handle for the IND.I/O expansion port pin D6/A7. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d6;

/** @brief Handle for the IND.I/O expansion port pin D7. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d7;

/** @brief Handle for the IND.I/O expansion port pin MISO/D14. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d14;

/** @brief Handle for the IND.I/O expansion port pin SCLK/D15. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d15;

/** @brief Handle for the IND.I/O expansion port pin MOSI/D16. */
extern platform_samd21g18a_pin_t const board_indio_expansion_d16;

/** @brief Handle for the IND.I/O analog output CH1. */
extern board_indio_analog_output_channel_t const board_indio_analog_output_ch1;

/** @brief Handle for the IND.I/O analog output CH2. */
extern board_indio_analog_output_channel_t const board_indio_analog_output_ch2;

/** @brief Handle for the IND.I/O analog input CH1. */
extern board_indio_analog_input_channel_t const board_indio_analog_input_ch1;

/** @brief Handle for the IND.I/O analog input CH2. */
extern board_indio_analog_input_channel_t const board_indio_analog_input_ch2;

/** @brief Handle for the IND.I/O analog input CH3. */
extern board_indio_analog_input_channel_t const board_indio_analog_input_ch3;

/** @brief Handle for the IND.I/O analog input CH4. */
extern board_indio_analog_input_channel_t const board_indio_analog_input_ch4;

#endif
