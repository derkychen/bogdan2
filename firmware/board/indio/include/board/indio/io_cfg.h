/**
 * @file io_cfg.h
 * @brief I/O configurations, directly mirrors wiring.
 *
 * This module exposes global handles for I/O through `extern` declarations that
 * can be passed into initialization functions.
 *
 * NOTE: This module only exposes the handles for I/O used in this project.
 */
#ifndef BOARD_INDIO_IO_CFG_H
#define BOARD_INDIO_IO_CFG_H

#include "board/indio/analog_output.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/pulser.h"

/** @brief Handle for the IND.I/O expansion port pin D4/A6 as a digital pin. */
extern digital_pin_t const io_cfg_expansion_d4_digital;

/** @brief Handle for the IND.I/O expansion port pin D2/SDA as a digital pin. */
extern digital_pin_t const io_cfg_expansion_d15_digital;

/** @brief Handle for the IND.I/O expansion port pin D5/PWM as an EIC pin. */
extern eic_pin_t const io_cfg_expansion_d5_eic;

/** @brief Handle for the IND.I/O expansion port pin D6/A7 as a EIC pin. */
extern eic_pin_t const io_cfg_expansion_d6_eic;

/** @brief Handle for the IND.I/O expansion port pin D3/SCL as a EIC pin. */
extern eic_pin_t const io_cfg_expansion_d16_eic;

/** @brief Handle for the IND.I/O expansion port pin D7 as a pulser. */
extern pulser_t const io_cfg_expansion_d7_pulser;

/** @brief Handle for the IND.I/O analog output CH1. */
extern analog_output_channel_t const io_cfg_analog_output_ch1;

/** @brief Handle for the IND.I/O analog output CH2. */
extern analog_output_channel_t const io_cfg_analog_output_ch2;

/** @brief Initialize the IND.I/O baseboard capabilities. */
void io_cfg_init(void);

#endif
