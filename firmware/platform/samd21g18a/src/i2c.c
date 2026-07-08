#include "platform/samd21g18a/i2c.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/pin.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCL_LATENCY_CYCLES (10U)
#define SCL_FREQUENCY_HZ   (PLATFORM_SAMD21G18A_I2C_SCL_FREQUENCY_STANDARD_HZ)
#define SCL_RISE_NSEC      (PLATFORM_SAMD21G18A_I2C_SCL_RISE_STANDARD_NSEC)

#define COMMAND_CONTINUE (2U)
#define COMMAND_STOP     (3U)

#define TIMEOUT_COUNT (100000U)

#define MAX_ADDRESS (0x7FU)

/** @brief Internal baud values structure. */
typedef struct
{
    /** SAMD21 BAUD register value for the number of cycles SCL is HIGH. */
    uint8_t baud;

    /** SAMD21 BAUD register value for the number of cycles SCL is LOW. */
    uint8_t baudlow;
} baud_t;

/**
 * @brief Internal I2C route structure.
 *
 * This is for storing valid I2C routes and helps with determining the correct
 * peripheral function to use in configuration.
 */
typedef struct
{
    /** Pin port group. */
    platform_samd21g18a_pin_port_group_t port_group;

    /** Pin number. */
    platform_samd21g18a_pin_number_t pin_number;

    /** Peripheral function for I2C. */
    platform_samd21g18a_pin_peripheral_function_t peripheral_function;

    /** I2C master. */
    platform_samd21g18a_i2c_master_t master;

    /** SERCOM pad. */
    platform_samd21g18a_i2c_sercom_pad_t pad;
} i2c_route_t;

/**
 * @brief Internal SERCOM structure.
 *
 * This is for data to be mapped to from the SERCOM enumeration.
 */
typedef struct
{
    /** SERCOM registers. */
    Sercom *sercom;

    /** APBC mask. */
    uint32_t apbc_mask;

    /** GCLK ID for the SERCOM instance. */
    uint16_t gclk_id;
} master_data_t;

static i2c_route_t const i2c_routes[] = {
    {
        .port_group          = 0U,
        .pin_number          = 16U,
        .peripheral_function = PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_C,
        .master              = PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM1,
        .pad                 = PLATFORM_SAMD21G18A_I2C_SERCOM_PAD0,
    },
    {
        .port_group          = 0U,
        .pin_number          = 17U,
        .peripheral_function = PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_C,
        .master              = PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM1,
        .pad                 = PLATFORM_SAMD21G18A_I2C_SERCOM_PAD1,
    },
};

static master_data_t const master_data[PLATFORM_SAMD21G18A_I2C_SERCOM_COUNT] = {
    [PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM0] = {
        .sercom    = SERCOM0,
        .apbc_mask = PM_APBCMASK_SERCOM0,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM0_CORE,
    },
    [PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM1] = {
        .sercom    = SERCOM1,
        .apbc_mask = PM_APBCMASK_SERCOM1,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM1_CORE,
    },
    [PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM2] = {
        .sercom    = SERCOM2,
        .apbc_mask = PM_APBCMASK_SERCOM2,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM2_CORE,
    },
    [PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM3] = {
        .sercom    = SERCOM3,
        .apbc_mask = PM_APBCMASK_SERCOM3,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM3_CORE,
    },
    [PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM4] = {
        .sercom    = SERCOM4,
        .apbc_mask = PM_APBCMASK_SERCOM4,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM4_CORE,
    },
    [PLATFORM_SAMD21G18A_I2C_MASTER_SERCOM5] = {
        .sercom    = SERCOM5,
        .apbc_mask = PM_APBCMASK_SERCOM5,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM5_CORE,
    },
};

/** @brief Ceiling division for `uint32_t`. */
static inline uint32_t
ceiling_divide (uint64_t numerator, uint32_t denominator)
{
    return (uint32_t)((numerator + (uint64_t)denominator - 1U)
                      / (uint64_t)denominator);
}

/** @brief Get the SERCOM registers for an I2C master. */
static inline Sercom *
master_get_sercom (platform_samd21g18a_i2c_master_t master)
{
    return master_data[master].sercom;
}

/** @brief Get the APBC mask for an I2C master. */
static inline uint32_t
master_get_apbc_mask (platform_samd21g18a_i2c_master_t master)
{
    return master_data[master].apbc_mask;
}

/** @brief Get the APBC mask for an I2C master. */
static inline uint16_t
master_get_gclk_id (platform_samd21g18a_i2c_master_t master)
{
    return master_data[master].gclk_id;
}

/** @brief Calculate BAUD and BAUDLOW. */
static void
baud_calculate (baud_t  *baud,
                uint32_t clock_frequency_hz,
                uint32_t scl_frequency_hz,
                uint32_t scl_rise_ns)
{
    uint32_t scl_period_cycles;
    uint32_t scl_rise_cycles;
    uint32_t programmable_cycles;

    PLATFORM_SAMD21G18A_ASSERT(baud != NULL);
    PLATFORM_SAMD21G18A_ASSERT((clock_frequency_hz != 0U)
                               && (scl_frequency_hz != 0U));

    scl_period_cycles = ceiling_divide(clock_frequency_hz, scl_frequency_hz);
    scl_rise_cycles   = ceiling_divide(
        (uint64_t)clock_frequency_hz * (uint64_t)scl_rise_ns, 1000000000U);

    PLATFORM_SAMD21G18A_ASSERT(scl_period_cycles
                               > (scl_rise_cycles + SCL_LATENCY_CYCLES));

    programmable_cycles
        = scl_period_cycles - scl_rise_cycles - SCL_LATENCY_CYCLES;

    PLATFORM_SAMD21G18A_ASSERT(programmable_cycles <= 510U);

    baud->baud    = (uint8_t)(programmable_cycles / 2U);
    baud->baudlow = (uint8_t)(programmable_cycles - baud->baud);

    return;
}

/** @brief Poll SERCOM I2C master until synchronized with a chosen mask. */
static inline void
sercom_poll_sync_mask (Sercom *sercom, uint32_t mask)
{
    while ((sercom->I2CM.SYNCBUSY.reg & mask) != 0U)
    {
    }

    return;
}

/** @brief Clear and set an I2C command. */
static inline void
sercom_set_command (Sercom *sercom, uint32_t command)
{
    sercom->I2CM.CTRLB.reg
        = (sercom->I2CM.CTRLB.reg & ~SERCOM_I2CM_CTRLB_CMD_Msk)
          | SERCOM_I2CM_CTRLB_CMD(command);

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

/** @brief Poll until the master is on the bus (ready for next command/byte). */
static platform_samd21g18a_i2c_status_t
sercom_poll_master_ready (Sercom *sercom)
{
    uint32_t timeout = TIMEOUT_COUNT;

    while ((sercom->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_MB) == 0U)
    {
        if ((sercom->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0U)
        {
            return PLATFORM_SAMD21G18A_I2C_STATUS_ERR;
        }

        if (timeout == 0U)
        {
            return PLATFORM_SAMD21G18A_I2C_STATUS_TIMEOUT;
        }

        timeout--;
    }

    if ((sercom->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK) != 0U)
    {
        return PLATFORM_SAMD21G18A_I2C_STATUS_NACK;
    }

    return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
}

/** @brief Poll until the slave is on the bus (ready for send byte). */
static platform_samd21g18a_i2c_status_t
sercom_poll_slave_ready (Sercom *sercom)
{
    uint32_t timeout = TIMEOUT_COUNT;

    while ((sercom->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_SB) == 0U)
    {
        if ((sercom->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0U)
        {
            return PLATFORM_SAMD21G18A_I2C_STATUS_ERR;
        }

        if (timeout == 0U)
        {
            return PLATFORM_SAMD21G18A_I2C_STATUS_TIMEOUT;
        }

        timeout--;
    }

    return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
}

/** @brief Terminate the message. */
static void
sercom_send_stop (Sercom *sercom)
{
    sercom_set_command(sercom, COMMAND_STOP);

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

static platform_samd21g18a_i2c_status_t
sercom_read_bytes (Sercom *sercom, uint8_t *data, size_t data_size)
{
    platform_samd21g18a_i2c_status_t status;
    size_t                           index;

    if (data_size == 0U)
    {
        return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
    }

    sercom->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    for (index = 0U; index < data_size; index++)
    {
        status = sercom_poll_slave_ready(sercom);

        if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
        {
            sercom_send_stop(sercom);
            return status;
        }

        if (index == (data_size - 1U))
        {
            sercom->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_ACKACT;
        }
        else
        {
            sercom->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
        }

        data[index] = (uint8_t)sercom->I2CM.DATA.reg;

        if (index == (data_size - 1U))
        {
            sercom_set_command(sercom, COMMAND_STOP);
        }
        else
        {
            sercom_set_command(sercom, COMMAND_CONTINUE);
        }
    }

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
}

/** @brief Resolve an I2C pin route to a PORT peripheral function. */
static platform_samd21g18a_pin_peripheral_function_t
pin_peripheral_function (platform_samd21g18a_i2c_pin_t const *i2c_pin,
                         platform_samd21g18a_i2c_master_t     master)
{
    PLATFORM_SAMD21G18A_ASSERT(i2c_pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(i2c_pin->pin != NULL);

    for (size_t i = 0U; i < sizeof(i2c_routes) / sizeof(i2c_routes[0]); i++)
    {
        if ((i2c_routes[i].port_group == i2c_pin->pin->port_group)
            && (i2c_routes[i].pin_number == i2c_pin->pin->number)
            && (i2c_routes[i].master == master)
            && (i2c_routes[i].pad == i2c_pin->pad))
        {
            return i2c_routes[i].peripheral_function;
        }
    }

    PLATFORM_SAMD21G18A_ASSERT(false);

    return PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_A;
}

/** @brief Configure an I2C pin. */
static void
pin_configure (platform_samd21g18a_i2c_pin_t const *i2c_pin,
               platform_samd21g18a_i2c_master_t     master)
{
    platform_samd21g18a_pin_peripheral_function_t peripheral_function;
    uint8_t                                       pmux_index;

    PLATFORM_SAMD21G18A_ASSERT(i2c_pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(i2c_pin->pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(i2c_pin->pin->port_group
                               < PLATFORM_SAMD21G18A_PIN_PORT_GROUP_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(i2c_pin->pin->number
                               < PLATFORM_SAMD21G18A_PIN_NUMBER_COUNT);

    peripheral_function = pin_peripheral_function(i2c_pin, master);

    pmux_index = i2c_pin->pin->number / 2U;

    // NOTE: No pull-up resistor is used because the IND.I/O board has them
    //       built in.
    PORT->Group[i2c_pin->pin->port_group].PINCFG[i2c_pin->pin->number].reg
        = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;

    if ((i2c_pin->pin->number & 1U) == 0U)
    {
        PORT->Group[i2c_pin->pin->port_group].PMUX[pmux_index].bit.PMUXE
            = peripheral_function;
    }
    else
    {
        PORT->Group[i2c_pin->pin->port_group].PMUX[pmux_index].bit.PMUXO
            = peripheral_function;
    }

    return;
}

void
platform_samd21g18a_i2c_configure (platform_samd21g18a_i2c_cfg_t const *cfg)
{
    uint32_t frequency_hz;
    uint32_t rise_nsec;
    baud_t   baud;
    Sercom  *sercom;

    PLATFORM_SAMD21G18A_ASSERT(cfg != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->master
                               < PLATFORM_SAMD21G18A_I2C_SERCOM_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(cfg->sda != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->scl != NULL);

    frequency_hz = SCL_FREQUENCY_HZ;
    rise_nsec    = SCL_RISE_NSEC;

    baud_calculate(&baud, SystemCoreClock, frequency_hz, rise_nsec);

    PM->APBCMASK.reg |= master_get_apbc_mask(cfg->master);

    GCLK->CLKCTRL.reg = master_get_gclk_id(cfg->master) | GCLK_CLKCTRL_GEN_GCLK0
                        | GCLK_CLKCTRL_CLKEN;

    platform_samd21g18a_utils_gclk_poll_sync();

    pin_configure(cfg->sda, cfg->master);
    pin_configure(cfg->scl, cfg->master);

    sercom = master_get_sercom(cfg->master);

    sercom->I2CM.CTRLA.bit.SWRST = 1U;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SWRST);

    sercom->I2CM.CTRLA.bit.ENABLE = 0U;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_ENABLE);

    sercom->I2CM.CTRLA.reg
        = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_SDAHOLD(2U);

    // Enable smart mode for handling ACK behaviour after DATA reads.
    sercom->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    sercom->I2CM.BAUD.bit.BAUD    = baud.baud;
    sercom->I2CM.BAUD.bit.BAUDLOW = baud.baudlow;

    sercom->I2CM.CTRLA.bit.ENABLE = 1U;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_ENABLE);

    sercom->I2CM.STATUS.bit.BUSSTATE = 1U;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

platform_samd21g18a_i2c_status_t
platform_samd21g18a_i2c_write (
    platform_samd21g18a_i2c_master_t        master,
    platform_samd21g18a_i2c_slave_address_t slave_address,
    uint8_t const                          *data,
    size_t                                  data_size)
{
    Sercom                          *sercom;
    platform_samd21g18a_i2c_status_t status;

    PLATFORM_SAMD21G18A_ASSERT(slave_address <= MAX_ADDRESS);
    PLATFORM_SAMD21G18A_ASSERT((data != NULL) || (data_size == 0U));

    sercom = master_get_sercom(master);

    sercom->I2CM.ADDR.reg = (uint32_t)((slave_address) << 1U);

    status = sercom_poll_master_ready(sercom);

    if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
    {
        sercom_send_stop(sercom);
        return status;
    }

    for (size_t index = 0U; index < data_size; index++)
    {
        sercom->I2CM.DATA.reg = data[index];

        status = sercom_poll_master_ready(sercom);

        if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
        {
            sercom_send_stop(sercom);

            return status;
        }
    }

    sercom_send_stop(sercom);

    return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
}

platform_samd21g18a_i2c_status_t
platform_samd21g18a_i2c_read (
    platform_samd21g18a_i2c_master_t        master,
    platform_samd21g18a_i2c_slave_address_t slave_address,
    uint8_t                                *data,
    size_t                                  data_size)
{
    Sercom *sercom;

    PLATFORM_SAMD21G18A_ASSERT(slave_address <= MAX_ADDRESS);
    PLATFORM_SAMD21G18A_ASSERT((data != NULL) || (data_size == 0U));

    if (data_size == 0U)
    {
        return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
    }

    sercom = master_get_sercom(master);

    sercom->I2CM.ADDR.reg = ((uint32_t)(((slave_address) << 1U) | 1U));

    return sercom_read_bytes(sercom, data, data_size);
}

platform_samd21g18a_i2c_status_t
platform_samd21g18a_i2c_write_read (
    platform_samd21g18a_i2c_master_t        master,
    platform_samd21g18a_i2c_slave_address_t slave_address,
    uint8_t const                          *write_data,
    size_t                                  write_data_size,
    uint8_t                                *read_data,
    size_t                                  read_data_size)
{
    Sercom                          *sercom;
    platform_samd21g18a_i2c_status_t status;

    PLATFORM_SAMD21G18A_ASSERT(slave_address <= MAX_ADDRESS);
    PLATFORM_SAMD21G18A_ASSERT((write_data != NULL) || (write_data_size == 0U));
    PLATFORM_SAMD21G18A_ASSERT((read_data != NULL) || (read_data_size == 0U));

    if (read_data_size == 0U)
    {
        return platform_samd21g18a_i2c_write(
            master, slave_address, write_data, write_data_size);
    }

    sercom = master_get_sercom(master);

    // First phase: START and REPEATED START.
    sercom->I2CM.ADDR.reg = (uint32_t)((slave_address) << 1U);

    status = sercom_poll_master_ready(sercom);

    if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
    {
        sercom_send_stop(sercom);
        return status;
    }

    for (size_t index = 0U; index < write_data_size; index++)
    {
        sercom->I2CM.DATA.reg = write_data[index];

        status = sercom_poll_master_ready(sercom);

        if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
        {
            sercom_send_stop(sercom);
            return status;
        }
    }

    // Second phase: REPEATED START and STOP.
    sercom->I2CM.ADDR.reg = ((uint32_t)(((slave_address) << 1U) | 1U));

    return sercom_read_bytes(sercom, read_data, read_data_size);
}
