/**
 * @file io_cfg.c
 * @brief Implementation of I/O configurations.
 *
 * NOTE: All MCU expansion port pins and analog output pins are defined. The
 *       ones that are unused are commented out.
 */
#include "board/indio/io_cfg.h"
#include "board/indio/analog_output.h"
#include "drivers/mcp4726.h"
#include "platform/samd21g18a/digital.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/i2c.h"

#if 0
/**
 * @brief Internal handle for the IND.I/O expansion port pin D0/RX.
 *
 * WARNING: The current module for MCU digital I/O does not support this pin.
 */
static pin_t const expansion_d0 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 19u,
};
#endif

#if 0
/**
 * @brief Internal handle for the IND.I/O expansion port pin D1/TX.
 *
 * WARNING: The current module for MCU digital I/O does not support this pin.
 */
static pin_t const expansion_d1 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 18u,
};
#endif

#if 0
/** @brief Internal handle for the IND.I/O expansion port pin D2/SDA. */
static pin_t const expansion_d2 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 16u,
};
#endif

#if 0
/** @brief Internal handle for the IND.I/O expansion port pin D3/SCL. */
static pin_t const expansion_d3 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 17u,
};
#endif

/** @brief Internal handle for the IND.I/O expansion port pin D4/A6. */
static pin_t const expansion_d4 = {
    .port_group = PIN_PORT_GROUP_B,
    .number     = 8u,
};

/** @brief Internal handle for the IND.I/O expansion port pin D5/PWM. */
static pin_t const expansion_d5 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 10u,
};

/** @brief Internal handle for the IND.I/O expansion port pin D6/A7. */
static pin_t const expansion_d6 = {
    .port_group = PIN_PORT_GROUP_B,
    .number     = 9u,
};

/** @brief Internal handle for the IND.I/O expansion port pin D7. */
static pin_t const expansion_d7 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 4u,
};

#if 0
/**
 * @brief Internal handle for the IND.I/O expansion port pin D10/A10.
 *
 * WARNING: The current module for MCU digital I/O does not support this pin.
 */
static pin_t const expansion_d10 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 7u,
};
#endif

#if 0
/** @brief Internal handle for the IND.I/O expansion port pin MISO/D14. */
static pin_t const expansion_d14 = {
    .port_group = PIN_PORT_GROUP_B,
    .number     = 22u,
};
#endif

/** @brief Internal handle for the IND.I/O expansion port pin SCLK/D15. */
static pin_t const expansion_d15 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 23u,
};

/** @brief Internal handle for the IND.I/O expansion port pin MOSI/D16. */
static pin_t const expansion_d16 = {
    .port_group = PIN_PORT_GROUP_B,
    .number     = 23u,
};

/**
 * @brief Internal PA16 handle.
 *
 * NOTE: This is the same pin as D2/SDA on the expansion port.
 */
static pin_t const pa16 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 16u,
};

/** @brief Internal PA17 handle.
 *
 * NOTE: This is the same pin as D3/SCL on the expansion port.
 */
static pin_t const pa17 = {
    .port_group = PIN_PORT_GROUP_A,
    .number     = 17u,
};

/**
 * @brief Internal handle for the SAMD21G18A pin corresponding to the I2C SDA on
 *        the IND.I/O.
 */
static i2c_pin_t const board_i2c_bus_sda = {
    .pin = &pa16,
    .pad = I2C_SERCOM_PAD0,
};

/**
 * @brief Internal handle for the SAMD21G18A pin corresponding to the I2C SCL on
 *        the IND.I/O.
 */
static i2c_pin_t const board_i2c_bus_scl = {
    .pin = &pa17,
    .pad = I2C_SERCOM_PAD1,
};

/** @brief Internal MCP4726 that controls all analog output CH1. */
static mcp4726_device_t const analog_output_mcp4726_ch1 = {
    .master  = I2C_MASTER_SERCOM1,
    .address = ANALOG_OUTPUT_CH1_MCP4726_ADDRESS,
};

/** @brief Internal MCP4726 that controls all analog output CH2. */
static mcp4726_device_t const analog_output_mcp4726_ch2 = {
    .master  = I2C_MASTER_SERCOM1,
    .address = ANALOG_OUTPUT_CH2_MCP4726_ADDRESS,
};

digital_pin_t const io_cfg_expansion_d4_digital = expansion_d4;

digital_pin_t const io_cfg_expansion_d15_digital = expansion_d15;

eic_pin_t const io_cfg_expansion_d5_eic = {
    .pin  = &expansion_d5,
    .line = 10u,
};

eic_pin_t const io_cfg_expansion_d6_eic = {
    .pin  = &expansion_d6,
    .line = 9u,
};

eic_pin_t const io_cfg_expansion_d16_eic = {
    .pin  = &expansion_d16,
    .line = 7u,
};

pulser_t const io_cfg_expansion_d7_pulser = {
    .output = &expansion_d7,
    .timer  = PULSER_TIMER_TCC0,
};

analog_output_channel_t const io_cfg_analog_output_ch1
    = analog_output_mcp4726_ch1;

analog_output_channel_t const io_cfg_analog_output_ch2
    = analog_output_mcp4726_ch2;

void
io_cfg_init (void)
{
    i2c_configure(I2C_MASTER_SERCOM1,
                  &board_i2c_bus_sda,
                  &board_i2c_bus_scl,
                  I2C_SCL_FREQUENCY_FAST_HZ,
                  I2C_SCL_RISE_FAST_NS);

    (void)analog_output_configure_v10();

    return;
}
