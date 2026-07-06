#ifndef BOARD_INDIO_ANALOG_INPUT_H
#define BOARD_INDIO_ANALOG_INPUT_H

#include "drivers/mcp3424.h"
#include <stddef.h>

#define BOARD_INDIO_ANALOG_INPUT_MCP3424_ADDRESS (0x68U)

#define BOARD_INDIO_ANALOG_INPUT_CH1_MCP3424_CHANNEL (DRIVERS_MCP3424_CHANNEL_1)
#define BOARD_INDIO_ANALOG_INPUT_CH2_MCP3424_CHANNEL (DRIVERS_MCP3424_CHANNEL_2)
#define BOARD_INDIO_ANALOG_INPUT_CH3_MCP3424_CHANNEL (DRIVERS_MCP3424_CHANNEL_3)
#define BOARD_INDIO_ANALOG_INPUT_CH4_MCP3424_CHANNEL (DRIVERS_MCP3424_CHANNEL_4)

/** @brief Analog output status codes. */
typedef enum
{
    BOARD_INDIO_ANALOG_INPUT_STATUS_OK = 0,
    BOARD_INDIO_ANALOG_INPUT_STATUS_ERR,
} board_indio_analog_input_status_t;

/** @brief Analog output type. */
typedef struct
{
    /** The MCP3424 device that the analog input corresponds to. */
    drivers_mcp3424_device_t const *device;

    /** The MCP3424 channel that the analog input corresponds to. */
    drivers_mcp3424_channel_t channel;

    /** The MCP3424 resolution for readings from the analog input. */
    drivers_mcp3424_resolution_t resolution;

    /** The MCP3424 gain for readings from the analog input. */
    drivers_mcp3424_gain_t gain;
} board_indio_analog_input_channel_t;

/** @brief Read a 32-bit signed integer from the MCP3424 device. */
board_indio_analog_input_status_t board_indio_analog_input_read(
    board_indio_analog_input_channel_t const *channel, int32_t *result);

#endif
