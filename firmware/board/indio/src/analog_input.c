#include "board/indio/analog_input.h"
#include "drivers/mcp3424.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/time.h"
#include <stdint.h>

#define READ_POLL_INTERVAL_MSEC (1U)

#define READ_TIMEOUT_12_BIT (10U)
#define READ_TIMEOUT_14_BIT (25U)
#define READ_TIMEOUT_16_BIT (80U)
#define READ_TIMEOUT_18_BIT (300U)

/** @brief Get the timeout limit in milliseconds for a certain resolution. */
static uint32_t
analog_input_timeout_msec (drivers_mcp3424_resolution_t resolution)
{
    switch (resolution)
    {
        case DRIVERS_MCP3424_RESOLUTION_12_BIT:
            return 10U;
        case DRIVERS_MCP3424_RESOLUTION_14_BIT:
            return 25U;
        case DRIVERS_MCP3424_RESOLUTION_16_BIT:
            return 80U;
        case DRIVERS_MCP3424_RESOLUTION_18_BIT:
        default:
            return 300U;
    }
}

board_indio_analog_input_status_t
board_indio_analog_input_read_raw (
    board_indio_analog_input_channel_t const *channel, int32_t *result)
{
    drivers_mcp3424_status_t status;
    uint32_t                 timeout_msec;
    uint32_t                 elapsed_msec;

    PLATFORM_SAMD21G18A_ASSERT(channel != NULL);
    PLATFORM_SAMD21G18A_ASSERT(result != NULL);

    status = drivers_mcp3424_start_conversion(
        channel->device, channel->channel, channel->resolution, channel->gain);

    if (status != DRIVERS_MCP3424_STATUS_OK)
    {
        return BOARD_INDIO_ANALOG_INPUT_STATUS_ERR;
    }

    timeout_msec = analog_input_timeout_msec(channel->resolution);
    elapsed_msec = 0U;

    while (elapsed_msec <= timeout_msec)
    {
        status = drivers_mcp3424_read(
            channel->device, channel->resolution, result);

        if (status == DRIVERS_MCP3424_STATUS_OK)
        {
            return BOARD_INDIO_ANALOG_INPUT_STATUS_OK;
        }
        else if (status != DRIVERS_MCP3424_STATUS_BUSY)
        {
            return BOARD_INDIO_ANALOG_INPUT_STATUS_ERR;
        }

        platform_samd21g18a_time_sleep_msec(READ_POLL_INTERVAL_MSEC);

        elapsed_msec += READ_POLL_INTERVAL_MSEC;
    }

    return BOARD_INDIO_ANALOG_INPUT_STATUS_ERR;
}
