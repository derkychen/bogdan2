#ifndef BOARD_INDIO_IO_CFG_H
#define BOARD_INDIO_IO_CFG_H

#include "board/indio/analog_output.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"

// NOTE: This module only exposes the handles for pins used in this project.
/** @brief Handle for the IND.I/O expansion port pin D4/A6 as a digital pin. */
extern platform_samd21g18a_digital_pin_t const
    board_indio_io_cfg_expansion_d4_digital;

/** @brief Handle for the IND.I/O expansion port pin D7 as a digital pin. */
extern platform_samd21g18a_digital_pin_t const
    board_indio_io_cfg_expansion_d7_digital;

/** @brief Handle for the IND.I/O expansion port pin D2/SDA as a digital pin. */
extern platform_samd21g18a_digital_pin_t const
    board_indio_io_cfg_expansion_d15_digital;

/** @brief Handle for the IND.I/O expansion port pin D5/PWM as an EIC pin. */
extern platform_samd21g18a_eic_pin_t const board_indio_io_cfg_expansion_d5_eic;

/** @brief Handle for the IND.I/O expansion port pin D6/A7 as a EIC pin. */
extern platform_samd21g18a_eic_pin_t const board_indio_io_cfg_expansion_d6_eic;

/** @brief Handle for the IND.I/O expansion port pin D3/SCL as a EIC pin. */
extern platform_samd21g18a_eic_pin_t const board_indio_io_cfg_expansion_d16_eic;

/** @brief Handle for the IND.I/O analog output CH1. */
extern board_indio_analog_output_channel_t const
    board_indio_io_cfg_analog_output_ch1;

/** @brief Handle for the IND.I/O analog output CH2. */
extern board_indio_analog_output_channel_t const
    board_indio_io_cfg_analog_output_ch2;

/** @brief Initialize the IND.I/O I/O to safe states. */
void board_indio_io_cfg_init(void);

#endif
