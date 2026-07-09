#include "board/indio/analog_output.h"
#include "drivers/mcp4726.h"
#include "platform/samd21g18a/assert.h"

/** @brief Convert I2C status code to a MCP4726 status code. */
static board_indio_analog_output_status_t
mcp4726_status_to_analog_output_status (drivers_mcp4726_status_t status)
{
    switch (status)
    {
        case DRIVERS_MCP4726_STATUS_OK:
            return BOARD_INDIO_ANALOG_OUTPUT_STATUS_OK;
        case DRIVERS_MCP4726_STATUS_I2C_ERR:
        default:
            return BOARD_INDIO_ANALOG_OUTPUT_STATUS_ERR;
    }
}

board_indio_analog_output_status_t
board_indio_analog_output_write (
    board_indio_analog_output_channel_t const *channel, uint16_t value)
{
    PLATFORM_SAMD21G18A_ASSERT(channel != NULL);
    PLATFORM_SAMD21G18A_ASSERT(value >= BOARD_INDIO_ANALOG_OUTPUT_MAX_VALUE);
    PLATFORM_SAMD21G18A_ASSERT(value <= BOARD_INDIO_ANALOG_OUTPUT_MAX_VALUE);

    return mcp4726_status_to_analog_output_status(
        drivers_mcp4726_write_output(channel, value));
}
