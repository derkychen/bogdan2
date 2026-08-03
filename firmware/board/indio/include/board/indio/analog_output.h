/**
 * @file analog_output.h
 * @brief This module provides all analog output functionality for the IND.I/O.
 */
#ifndef BOARD_INDIO_ANALOG_OUTPUT_H
#define BOARD_INDIO_ANALOG_OUTPUT_H

#include "drivers/mcp4726.h"
#include <stddef.h>

#define ANALOG_OUTPUT_MAX_VALUE (MCP4726_MAX_VALUE)

#define ANALOG_OUTPUT_CH1_MCP4726_ADDRESS (0x60u)
#define ANALOG_OUTPUT_CH2_MCP4726_ADDRESS (0x61u)

/** @brief Analog output status codes. */
typedef enum
{
    ANALOG_OUTPUT_STATUS_OK = 0,
    ANALOG_OUTPUT_STATUS_ERR_CFG,
    ANALOG_OUTPUT_STATUS_ERR_WRITE,
} analog_output_status_t;

/** @brief Analog output type. */
typedef mcp4726_device_t analog_output_channel_t;

/** @brief Configure IND.I/O analog outputs CH1 and CH2 for 0-10 V. */
analog_output_status_t analog_output_configure_v10(void);

/** @brief Write a 16-bit value to an MCP4726 device. */
analog_output_status_t analog_output_write(
    analog_output_channel_t const *channel, uint16_t value);

#endif
