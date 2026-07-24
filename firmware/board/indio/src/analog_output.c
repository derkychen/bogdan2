#include "board/indio/analog_output.h"
#include "drivers/mcp4726.h"
#include "drivers/pca9555.h"
#include "platform/samd21g18a/assert.h"

#define MODE_EXPANDER_ADDRESS (0x20u)
#define MODE_CH1_MASK         (1u << 8u)
#define MODE_CH2_MASK         (1u << 9u)
#define MODE_CFGS_DEFAULT     (0xFFFFu)
#define MODE_OUTPUTS_DEFAULT  (0x0000u)
#define MODE_OUTPUTS_MASK     (MODE_CH1_MASK | MODE_CH2_MASK)

static drivers_pca9555_device_t const analog_mode_expander = {
    .master  = PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM1,
    .address = MODE_EXPANDER_ADDRESS,
};

static drivers_pca9555_cfgs_t    mode_cfgs_shadow    = MODE_CFGS_DEFAULT;
static drivers_pca9555_outputs_t mode_outputs_shadow = MODE_OUTPUTS_DEFAULT;

static board_indio_analog_output_status_t
pca9555_status_to_analog_output_status(drivers_pca9555_status_t status);

static board_indio_analog_output_status_t
mcp4726_status_to_analog_output_status(drivers_mcp4726_status_t status);

board_indio_analog_output_status_t
board_indio_analog_output_configure_v10 (void)
{
    drivers_pca9555_status_t status;

    // Set mode configurations for both channels to be outputs.
    mode_cfgs_shadow
        = (drivers_pca9555_cfgs_t)(mode_cfgs_shadow
                                   & (drivers_pca9555_cfgs_t)(~MODE_OUTPUTS_MASK));

    status
        = drivers_pca9555_write_cfgs(&analog_mode_expander, mode_cfgs_shadow);

    if (status != DRIVERS_PCA9555_STATUS_OK)
    {
        return pca9555_status_to_analog_output_status(status);
    }

    // Clear mode expander output bits for voltage mode.
    mode_outputs_shadow
        = (drivers_pca9555_outputs_t)(mode_outputs_shadow
                                      & (drivers_pca9555_outputs_t)(~MODE_OUTPUTS_MASK));

    status = drivers_pca9555_write_outputs(&analog_mode_expander,
                                           mode_outputs_shadow);

    return pca9555_status_to_analog_output_status(status);
}

board_indio_analog_output_status_t
board_indio_analog_output_write (
    board_indio_analog_output_channel_t const *channel, uint16_t value)
{
    PLATFORM_SAMD21G18A_ASSERT(channel != NULL);
    PLATFORM_SAMD21G18A_ASSERT(value <= BOARD_INDIO_ANALOG_OUTPUT_MAX_VALUE);

    return mcp4726_status_to_analog_output_status(
        drivers_mcp4726_write_output(channel, value));
}

/** @brief Convert a PCA9555 status code to a analog output status code. */
static board_indio_analog_output_status_t
pca9555_status_to_analog_output_status (drivers_pca9555_status_t status)
{
    switch (status)
    {
        case DRIVERS_PCA9555_STATUS_OK:
            return BOARD_INDIO_ANALOG_OUTPUT_STATUS_OK;

        case DRIVERS_PCA9555_STATUS_ERR:
        default:
            return BOARD_INDIO_ANALOG_OUTPUT_STATUS_ERR_CFG;
    }
}

/** @brief Convert a MCP4726 status code to an analog output status code. */
static board_indio_analog_output_status_t
mcp4726_status_to_analog_output_status (drivers_mcp4726_status_t status)
{
    switch (status)
    {
        case DRIVERS_MCP4726_STATUS_OK:
            return BOARD_INDIO_ANALOG_OUTPUT_STATUS_OK;
        case DRIVERS_MCP4726_STATUS_ERR:
        default:
            return BOARD_INDIO_ANALOG_OUTPUT_STATUS_ERR_WRITE;
    }
}
