#include "board/indio/io_cfg.h"
#include "drivers/mcp3424.h"
#include "drivers/mcp4726.h"
#include "platform/samd21g18a/i2c.h"
#include "platform/samd21g18a/pin.h"

#define SAMD21G18A_PORT_GROUP_A          (0U)
#define SAMD21G18A_PORT_GROUP_B          (1U)
#define SAMD21G18A_PERIPHERAL_FUNCTION_A (0U)
#define SAMD21G18A_PERIPHERAL_FUNCTION_C (2U)

platform_samd21g18a_pin_t const board_indio_expansion_d0 = {
    .port_group          = SAMD21G18A_PORT_GROUP_A,
    .number              = 19U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

platform_samd21g18a_pin_t const board_indio_expansion_d1 = {
    .port_group          = SAMD21G18A_PORT_GROUP_A,
    .number              = 18U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

platform_samd21g18a_pin_t const board_indio_expansion_d2 = {
    .port_group          = SAMD21G18A_PORT_GROUP_A,
    .number              = 16U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_C,
};

platform_samd21g18a_pin_t const board_indio_expansion_d3 = {
    .port_group          = SAMD21G18A_PORT_GROUP_A,
    .number              = 17U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_C,
};

platform_samd21g18a_pin_t const board_indio_expansion_d4 = {
    .port_group          = SAMD21G18A_PORT_GROUP_B,
    .number              = 8U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

platform_samd21g18a_pin_t const board_indio_expansion_d5 = {
    .port_group          = SAMD21G18A_PORT_GROUP_A,
    .number              = 10U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

platform_samd21g18a_pin_t const board_indio_expansion_d6 = {
    .port_group          = SAMD21G18A_PORT_GROUP_B,
    .number              = 9U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

platform_samd21g18a_pin_t const board_indio_expansion_d7 = {
    .port_group          = SAMD21G18A_PORT_GROUP_A,
    .number              = 4U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

platform_samd21g18a_pin_t const board_indio_expansion_d10 = {
    .port_group          = SAMD21G18A_PORT_GROUP_A,
    .number              = 7U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

platform_samd21g18a_pin_t const board_indio_expansion_d14 = {
    .port_group          = SAMD21G18A_PORT_GROUP_B,
    .number              = 22U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

platform_samd21g18a_pin_t const board_indio_expansion_d15 = {
    .port_group          = SAMD21G18A_PORT_GROUP_A,
    .number              = 23U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

platform_samd21g18a_pin_t const board_indio_expansion_d16 = {
    .port_group          = SAMD21G18A_PORT_GROUP_B,
    .number              = 23U,
    .peripheral_function = SAMD21G18A_PERIPHERAL_FUNCTION_A,
};

/* @brief Internal MCP4726 that controls all analog output CH1. */
static drivers_mcp4726_device_t const analog_output_mcp4726_ch1 = {
    .master  = PLATFORM_SAMD21G18A_I2C_MASTER,
    .address = BOARD_INDIO_ANALOG_OUTPUT_CH1_MCP4726_ADDRESS,
};

/* @brief Internal MCP4726 that controls all analog output CH2. */
static drivers_mcp4726_device_t const analog_output_mcp4726_ch2 = {
    .master  = PLATFORM_SAMD21G18A_I2C_MASTER,
    .address = BOARD_INDIO_ANALOG_OUTPUT_CH2_MCP4726_ADDRESS,
};

/* @brief Internal MCP3424 that controls all four channels. */
static drivers_mcp3424_device_t const analog_input_mcp3424 = {
    .master  = PLATFORM_SAMD21G18A_I2C_MASTER,
    .address = BOARD_INDIO_ANALOG_INPUT_MCP3424_ADDRESS,
};

board_indio_analog_output_channel_t const board_indio_analog_output_ch1
    = analog_output_mcp4726_ch1;

board_indio_analog_output_channel_t const board_indio_analog_output_ch2
    = analog_output_mcp4726_ch2;

board_indio_analog_input_channel_t const board_indio_analog_input_ch1 = {
    .device     = &analog_input_mcp3424,
    .channel    = DRIVERS_MCP3424_CHANNEL_1,
    .resolution = DRIVERS_MCP3424_RESOLUTION_14_BIT,
    .gain       = DRIVERS_MCP3424_GAIN_1X,
};

board_indio_analog_input_channel_t const board_indio_analog_input_ch2 = {
    .device     = &analog_input_mcp3424,
    .channel    = DRIVERS_MCP3424_CHANNEL_2,
    .resolution = DRIVERS_MCP3424_RESOLUTION_14_BIT,
    .gain       = DRIVERS_MCP3424_GAIN_1X,
};

board_indio_analog_input_channel_t const board_indio_analog_input_ch3 = {
    .device     = &analog_input_mcp3424,
    .channel    = DRIVERS_MCP3424_CHANNEL_3,
    .resolution = DRIVERS_MCP3424_RESOLUTION_14_BIT,
    .gain       = DRIVERS_MCP3424_GAIN_1X,
};

board_indio_analog_input_channel_t const board_indio_analog_input_ch4 = {
    .device     = &analog_input_mcp3424,
    .channel    = DRIVERS_MCP3424_CHANNEL_4,
    .resolution = DRIVERS_MCP3424_RESOLUTION_14_BIT,
    .gain       = DRIVERS_MCP3424_GAIN_1X,
};
