#include "board/indio/io_cfg.h"
#include "board/indio/analog_output.h"
#include "drivers/mcp4726.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/i2c.h"

#define SAMD21G18A_PORT_GROUP_A          (0U)
#define SAMD21G18A_PORT_GROUP_B          (1U)
#define SAMD21G18A_PERIPHERAL_FUNCTION_A (0U)
#define SAMD21G18A_PERIPHERAL_FUNCTION_C (2U)

// TODO: Remove unneeded internal pin structures if needed.
#if 0
/**
 * @brief Internal handle for the IND.I/O expansion port pin D0/RX.
 *
 * WARNING: The current module for MCU digital I/O does not support this pin.
 */
static platform_samd21g18a_pin_t const expansion_d0 = {
    .port_group = SAMD21G18A_PORT_GROUP_A,
    .number     = 19U,
};
#endif

#if 0
/**
 * @brief Internal handle for the IND.I/O expansion port pin D1/TX.
 *
 * WARNING: The current module for MCU digital I/O does not support this pin.
 */
static platform_samd21g18a_pin_t const expansion_d1 = {
    .port_group = SAMD21G18A_PORT_GROUP_A,
    .number     = 18U,
};
#endif

/** @brief Internal handle for the IND.I/O expansion port pin D2/SDA. */
static platform_samd21g18a_pin_t const expansion_d2 = {
    .port_group = SAMD21G18A_PORT_GROUP_A,
    .number     = 16U,
};

#if 0
/** @brief Internal handle for the IND.I/O expansion port pin D3/SCL. */
static platform_samd21g18a_pin_t const expansion_d3 = {
    .port_group = SAMD21G18A_PORT_GROUP_A,
    .number     = 17U,
};
#endif

/** @brief Internal handle for the IND.I/O expansion port pin D4/A6. */
static platform_samd21g18a_pin_t const expansion_d4 = {
    .port_group = SAMD21G18A_PORT_GROUP_B,
    .number     = 8U,
};

/** @brief Internal handle for the IND.I/O expansion port pin D5/PWM. */
static platform_samd21g18a_pin_t const expansion_d5 = {
    .port_group = SAMD21G18A_PORT_GROUP_A,
    .number     = 10U,
};

/** @brief Internal handle for the IND.I/O expansion port pin D6/A7. */
static platform_samd21g18a_pin_t const expansion_d6 = {
    .port_group = SAMD21G18A_PORT_GROUP_B,
    .number     = 9U,
};

/** @brief Internal handle for the IND.I/O expansion port pin D7. */
static platform_samd21g18a_pin_t const expansion_d7 = {
    .port_group = SAMD21G18A_PORT_GROUP_A,
    .number     = 4U,
};

#if 0
/**
 * @brief Internal handle for the IND.I/O expansion port pin D10/A10.
 *
 * WARNING: The current module for MCU digital I/O does not support this pin.
 */
static platform_samd21g18a_pin_t const expansion_d10 = {
    .port_group = SAMD21G18A_PORT_GROUP_A,
    .number     = 7U,
};
#endif

#if 0
/** @brief Internal handle for the IND.I/O expansion port pin MISO/D14. */
static platform_samd21g18a_pin_t const expansion_d14 = {
    .port_group = SAMD21G18A_PORT_GROUP_B,
    .number     = 22U,
};
#endif

#if 0
/** @brief Internal handle for the IND.I/O expansion port pin SCLK/D15. */
static platform_samd21g18a_pin_t const expansion_d15 = {
    .port_group = SAMD21G18A_PORT_GROUP_A,
    .number     = 23U,
};
#endif

#if 0
/** @brief Internal handle for the IND.I/O expansion port pin MOSI/D16. */
static platform_samd21g18a_pin_t const expansion_d16 = {
    .port_group = SAMD21G18A_PORT_GROUP_B,
    .number     = 23U,
};
#endif

/** @brief Internal PA16 handle. */
static platform_samd21g18a_pin_t const pa16 = {
    .port_group = PLATFORM_SAMD21G18A_PIN_PORT_GROUP_A,
    .number     = 16U,
};

/** @brief Internal PA17 handle. */
static platform_samd21g18a_pin_t const pa17 = {
    .port_group = PLATFORM_SAMD21G18A_PIN_PORT_GROUP_A,
    .number     = 17U,
};

/** @brief Internal MCP4726 that controls all analog output CH1. */
static drivers_mcp4726_device_t const analog_output_mcp4726_ch1 = {
    .master  = PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM1,
    .address = BOARD_INDIO_ANALOG_OUTPUT_CH1_MCP4726_ADDRESS,
};

/** @brief Internal MCP4726 that controls all analog output CH2. */
static drivers_mcp4726_device_t const analog_output_mcp4726_ch2 = {
    .master  = PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM1,
    .address = BOARD_INDIO_ANALOG_OUTPUT_CH2_MCP4726_ADDRESS,
};

platform_samd21g18a_digital_pin_t const board_indio_expansion_d4_digital
    = expansion_d4;

platform_samd21g18a_digital_pin_t const board_indio_expansion_d5_digital
    = expansion_d5;

platform_samd21g18a_eic_pin_t const board_indio_expansion_d2_eic = {
    .pin  = &expansion_d2,
    .line = 0U,
};

platform_samd21g18a_eic_pin_t const board_indio_expansion_d7_eic = {
    .pin  = &expansion_d7,
    .line = 4U,
};

platform_samd21g18a_eic_pin_t const board_indio_expansion_d6_eic = {
    .pin  = &expansion_d6,
    .line = 9U,
};

platform_samd21g18a_i2c_pin_t const board_indio_bus_sda = {
    .pin = &pa16,
    .pad = PLATFORM_SAMD21G18A_I2C_SERCOM_PAD0,
};

platform_samd21g18a_i2c_pin_t const board_indio_bus_scl = {
    .pin = &pa17,
    .pad = PLATFORM_SAMD21G18A_I2C_SERCOM_PAD1,
};

board_indio_analog_output_channel_t const board_indio_analog_output_ch1
    = analog_output_mcp4726_ch1;

board_indio_analog_output_channel_t const board_indio_analog_output_ch2
    = analog_output_mcp4726_ch2;
