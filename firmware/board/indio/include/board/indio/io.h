/**
 * @file io.h
 * @brief I/O configurations, directly mirrors wiring.
 *
 * This module exposes global handles for I/O through `extern` declarations that
 * can be passed into initialization functions.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 *
 * NOTE: This module only exposes the handles for I/O used in this project.
 */
#ifndef BOARD_INDIO_IO_H
#define BOARD_INDIO_IO_H

#include "board/indio/analog_output.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/pulser.h"

/** @brief I/O configuration status codes. */
typedef enum
{
    IO_STATUS_OK = 0,
    IO_STATUS_ERR,
} io_status_t;

/** @brief Handle for the IND.I/O expansion port pin D4/A6 as a digital pin. */
extern digital_pin_t const io_expansion_d4_digital;

/** @brief Handle for the IND.I/O expansion port pin D2/SDA as a digital pin. */
extern digital_pin_t const io_expansion_d15_digital;

/** @brief Handle for the IND.I/O expansion port pin D5/PWM as an EIC pin. */
extern eic_pin_t const io_expansion_d5_eic;

/** @brief Handle for the IND.I/O expansion port pin D6/A7 as a EIC pin. */
extern eic_pin_t const io_expansion_d6_eic;

/** @brief Handle for the IND.I/O expansion port pin D3/SCL as a EIC pin. */
extern eic_pin_t const io_expansion_d16_eic;

/** @brief Handle for the IND.I/O expansion port pin D7 as a pulser. */
extern pulser_t const io_expansion_d7_pulser;

/** @brief Handle for the IND.I/O analog output CH1. */
extern analog_output_channel_t const io_analog_output_ch1;

/** @brief Handle for the IND.I/O analog output CH2. */
extern analog_output_channel_t const io_analog_output_ch2;

/**
 * @brief Initialize the IND.I/O baseboard capabilities.
 *
 * Configures an I2C bus on PA16 and PA17 with SERCOM1 as the I2C master.
 */
void io_init(void);

/**
 * @brief Configure the IND.I/O outputs.
 *
 * Currently only configures analog outputs to produce 0 to 10 volts.
 *
 * NOTE: This function must be called if the baseboard is power cycled, as the
 *       voltage range configuration is lost each time. This can happen when the
 *       baseboard is unplugged while the processor is powered. Processor I/O do
 *       not need to be handled by this function since a power cycle of the
 *       processor means a clean reset.
 */
io_status_t io_configure(void);

#endif
