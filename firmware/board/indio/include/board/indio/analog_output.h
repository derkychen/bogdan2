#ifndef BOARD_INDIO_ANALOG_OUTPUT_H
#define BOARD_INDIO_ANALOG_OUTPUT_H

#include "drivers/mcp4726.h"
#include <stddef.h>

#define BOARD_INDIO_ANALOG_OUTPUT_MAX_VALUE (DRIVERS_MCP4726_MAX_VALUE)

#define BOARD_INDIO_ANALOG_OUTPUT_CH1_MCP4726_ADDRESS (0x60)
#define BOARD_INDIO_ANALOG_OUTPUT_CH2_MCP4726_ADDRESS (0x61)

/** @brief Analog output status codes. */
typedef enum
{
    BOARD_INDIO_ANALOG_OUTPUT_STATUS_OK = 0,
    BOARD_INDIO_ANALOG_OUTPUT_STATUS_ERR_CFG,
    BOARD_INDIO_ANALOG_OUTPUT_STATUS_ERR_WRITE,
} board_indio_analog_output_status_t;

/** @brief Analog output type. */
typedef drivers_mcp4726_device_t board_indio_analog_output_channel_t;

/** @brief Configure IND.I/O analog outputs CH1 and CH2 for 0-10 V. */
board_indio_analog_output_status_t board_indio_analog_output_configure_v10(
    void);

/** @brief Write a 16-bit value to an MCP4726 device. */
board_indio_analog_output_status_t board_indio_analog_output_write(
    board_indio_analog_output_channel_t const *channel, uint16_t value);

#endif
