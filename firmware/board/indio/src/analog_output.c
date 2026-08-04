/**
 * @file analog_output.c
 * @brief Implementation of analog output functionality for the IND.I/O.
 *
 * NOTE: The MCP4726 is the chip used to actually perform DAC conversions.
 *       However, the PCA9555 driver is still needed in the configuration of the
 *       output voltage range to 0 to 10 volts.
 */
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

static pca9555_device_t const analog_mode_expander = {
    .master  = I2C_MASTER_SERCOM1,
    .address = MODE_EXPANDER_ADDRESS,
};

static pca9555_cfgs_t    mode_cfgs_shadow    = MODE_CFGS_DEFAULT;
static pca9555_outputs_t mode_outputs_shadow = MODE_OUTPUTS_DEFAULT;

static analog_output_status_t pca9555_status_to_analog_output_status(
    pca9555_status_t status);
static analog_output_status_t mcp4726_status_to_analog_output_status(
    mcp4726_status_t status);

analog_output_status_t
analog_output_configure_v10 (void)
{
    pca9555_status_t status;

    // Set mode configurations for both channels to be outputs.
    mode_cfgs_shadow = (pca9555_cfgs_t)(mode_cfgs_shadow
                                        & (pca9555_cfgs_t)(~MODE_OUTPUTS_MASK));

    status = pca9555_write_cfgs(&analog_mode_expander, mode_cfgs_shadow);

    if (status != PCA9555_STATUS_OK)
    {
        return pca9555_status_to_analog_output_status(status);
    }

    // Clear mode expander output bits for voltage mode.
    mode_outputs_shadow
        = (pca9555_outputs_t)(mode_outputs_shadow
                              & (pca9555_outputs_t)(~MODE_OUTPUTS_MASK));

    status = pca9555_write_outputs(&analog_mode_expander, mode_outputs_shadow);

    return pca9555_status_to_analog_output_status(status);
}

analog_output_status_t
analog_output_write (analog_output_channel_t const *channel, uint16_t value)
{
    ASSERT(channel != NULL);
    ASSERT(value <= ANALOG_OUTPUT_MAX_VALUE);

    uint16_t write_value = (value & ANALOG_OUTPUT_MAX_VALUE);

    return mcp4726_status_to_analog_output_status(
        mcp4726_write_output(channel, write_value));
}

/** @brief Convert a PCA9555 status code to a analog output status code. */
static analog_output_status_t
pca9555_status_to_analog_output_status (pca9555_status_t status)
{
    switch (status)
    {
        case PCA9555_STATUS_OK:
            return ANALOG_OUTPUT_STATUS_OK;

        case PCA9555_STATUS_ERR:
        default:
            return ANALOG_OUTPUT_STATUS_ERR_CFG;
    }
}

/** @brief Convert a MCP4726 status code to an analog output status code. */
static analog_output_status_t
mcp4726_status_to_analog_output_status (mcp4726_status_t status)
{
    switch (status)
    {
        case MCP4726_STATUS_OK:
            return ANALOG_OUTPUT_STATUS_OK;
        case MCP4726_STATUS_ERR:
        default:
            return ANALOG_OUTPUT_STATUS_ERR_WRITE;
    }
}
