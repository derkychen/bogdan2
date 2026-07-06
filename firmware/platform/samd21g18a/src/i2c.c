#include "platform/samd21g18a/i2c.h"
#include "platform/samd21g18a/utils.h"
#include "platform/samd21g18a/assert.h"
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

/** @brief Baud values structure. */
typedef struct
{
    /** SAMD21 BAUD register value for the number of cycles SCL is HIGH. */
    uint8_t baud;

    /** SAMD21 BAUD register value for the number of cycles SCL is LOW. */
    uint8_t baudlow;
} baud_t;

/** @brief Ceiling division for `uint32_t`. */
static inline uint32_t
ceiling_divide (uint64_t numerator, uint32_t denominator)
{
    return (uint32_t)((numerator + (uint64_t)denominator - 1U)
                      / (uint64_t)denominator);
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
master_poll_sync_mask (platform_samd21g18a_i2c_master_t *master, uint32_t mask)
{
    while ((master->I2CM.SYNCBUSY.reg & mask) != 0U)
    {
    }

    return;
}

/** @brief Enable the clock of a SERCOM. */
static void
master_enable_clock (platform_samd21g18a_i2c_master_t *master)
{
    PLATFORM_SAMD21G18A_ASSERT(master == SERCOM1);

    if (master == SERCOM1)
    {
        PM->APBCMASK.reg |= PM_APBCMASK_SERCOM1;

        GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_SERCOM1_CORE
                            | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;

        platform_samd21g18a_utils_gclk_poll_sync();
    }

    return;
}

/** @brief Clear and set an I2C command. */
static inline void
master_set_command (platform_samd21g18a_i2c_master_t *master, uint32_t command)
{
    master->I2CM.CTRLB.reg
        = (master->I2CM.CTRLB.reg & ~SERCOM_I2CM_CTRLB_CMD_Msk)
          | SERCOM_I2CM_CTRLB_CMD(command);

    master_poll_sync_mask(master, SERCOM_I2CM_SYNCBUSY_SYSOP);
}

/** @brief Poll until the master is on the bus (ready for next command/byte). */
static platform_samd21g18a_i2c_status_t
master_poll_until_ready (platform_samd21g18a_i2c_master_t *master)
{
    uint32_t timeout = TIMEOUT_COUNT;

    while ((master->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_MB) == 0U)
    {
        if ((master->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0U)
        {
            return PLATFORM_SAMD21G18A_I2C_STATUS_ERR;
        }

        if (timeout == 0U)
        {
            return PLATFORM_SAMD21G18A_I2C_STATUS_TIMEOUT;
        }

        timeout--;
    }

    if ((master->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK) != 0U)
    {
        return PLATFORM_SAMD21G18A_I2C_STATUS_NACK;
    }

    return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
}

/** @brief Poll until the slave is on the bus (ready for send byte). */
static platform_samd21g18a_i2c_status_t
master_poll_until_slave_ready (platform_samd21g18a_i2c_master_t *master)
{
    uint32_t timeout = TIMEOUT_COUNT;

    while ((master->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_SB) == 0U)
    {
        if ((master->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0U)
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
master_send_stop (platform_samd21g18a_i2c_master_t *master)
{
    master_set_command(master, COMMAND_STOP);

    master_poll_sync_mask(master, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

static platform_samd21g18a_i2c_status_t
master_read_bytes (platform_samd21g18a_i2c_master_t *master,
                   uint8_t                          *data,
                   size_t                            data_size)
{
    platform_samd21g18a_i2c_status_t status;
    size_t                           index;

    if (data_size == 0U)
    {
        return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
    }

    master->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
    master_poll_sync_mask(master, SERCOM_I2CM_SYNCBUSY_SYSOP);

    for (index = 0U; index < data_size; index++)
    {
        status = master_poll_until_slave_ready(master);

        if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
        {
            master_send_stop(master);
            return status;
        }

        if (index == (data_size - 1U))
        {
            master->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_ACKACT;
        }
        else
        {
            master->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
        }

        data[index] = (uint8_t)master->I2CM.DATA.reg;

        if (index == (data_size - 1U))
        {
            master_set_command(master, COMMAND_STOP);
        }
        else
        {
            master_set_command(master, COMMAND_CONTINUE);
        }
    }

    master_poll_sync_mask(master, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
}

/** @brief Configure an I2C pin. */
static void
pin_configure (platform_samd21g18a_pin_t const *pin)
{
    uint8_t pmux_index;

    PLATFORM_SAMD21G18A_ASSERT(pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(pin->port_group <= 1U);
    PLATFORM_SAMD21G18A_ASSERT(pin->number <= 31U);
    PLATFORM_SAMD21G18A_ASSERT(pin->peripheral_function <= 7U);

    pmux_index = pin->number / 2U;

    // NOTE: No pull-up resistor is used because the IND.I/O board has them
    //       built in.
    PORT->Group[pin->port_group].PINCFG[pin->number].reg
        = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;

    if ((pin->number & 1U) == 0U)
    {
        PORT->Group[pin->port_group].PMUX[pmux_index].bit.PMUXE
            = pin->peripheral_function;
    }
    else
    {
        PORT->Group[pin->port_group].PMUX[pmux_index].bit.PMUXO
            = pin->peripheral_function;
    }

    return;
}

void
platform_samd21g18a_i2c_configure (platform_samd21g18a_i2c_cfg_t const *cfg)
{
    uint32_t frequency_hz;
    uint32_t rise_nsec;
    baud_t   baud;

    PLATFORM_SAMD21G18A_ASSERT(cfg != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->master != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->sda != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->scl != NULL);

    frequency_hz = SCL_FREQUENCY_HZ;
    rise_nsec    = SCL_RISE_NSEC;

    baud_calculate(&baud, SystemCoreClock, frequency_hz, rise_nsec);

    master_enable_clock(cfg->master);

    pin_configure(cfg->sda);
    pin_configure(cfg->scl);

    cfg->master->I2CM.CTRLA.bit.SWRST = 1U;

    master_poll_sync_mask(cfg->master, SERCOM_I2CM_SYNCBUSY_SWRST);

    cfg->master->I2CM.CTRLA.bit.ENABLE = 0U;

    master_poll_sync_mask(cfg->master, SERCOM_I2CM_SYNCBUSY_ENABLE);

    cfg->master->I2CM.CTRLA.reg
        = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_SDAHOLD(2U);

    // Enable smart mode for handling ACK behaviour after DATA reads.
    cfg->master->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;

    master_poll_sync_mask(cfg->master, SERCOM_I2CM_SYNCBUSY_SYSOP);

    cfg->master->I2CM.BAUD.bit.BAUD    = baud.baud;
    cfg->master->I2CM.BAUD.bit.BAUDLOW = baud.baudlow;

    cfg->master->I2CM.CTRLA.bit.ENABLE = 1U;

    master_poll_sync_mask(cfg->master, SERCOM_I2CM_SYNCBUSY_ENABLE);

    cfg->master->I2CM.STATUS.bit.BUSSTATE = 1U;

    master_poll_sync_mask(cfg->master, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

platform_samd21g18a_i2c_status_t
platform_samd21g18a_i2c_write (
    platform_samd21g18a_i2c_master_t       *master,
    platform_samd21g18a_i2c_slave_address_t slave_address,
    uint8_t const                          *data,
    size_t                                  data_size)
{
    platform_samd21g18a_i2c_status_t status;

    PLATFORM_SAMD21G18A_ASSERT(slave_address <= MAX_ADDRESS);
    PLATFORM_SAMD21G18A_ASSERT((data != NULL) || (data_size == 0U));

    master->I2CM.ADDR.reg = (uint32_t)((slave_address) << 1U);

    status = master_poll_until_ready(master);

    if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
    {
        master_send_stop(master);
        return status;
    }

    for (size_t index = 0U; index < data_size; index++)
    {
        master->I2CM.DATA.reg = data[index];

        status = master_poll_until_ready(master);

        if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
        {
            master_send_stop(master);

            return status;
        }
    }

    master_send_stop(master);

    return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
}

platform_samd21g18a_i2c_status_t
platform_samd21g18a_i2c_read (
    platform_samd21g18a_i2c_master_t       *master,
    platform_samd21g18a_i2c_slave_address_t slave_address,
    uint8_t                                *data,
    size_t                                  data_size)
{
    PLATFORM_SAMD21G18A_ASSERT(slave_address <= MAX_ADDRESS);
    PLATFORM_SAMD21G18A_ASSERT((data != NULL) || (data_size == 0U));

    if (data_size == 0U)
    {
        return PLATFORM_SAMD21G18A_I2C_STATUS_OK;
    }

    master->I2CM.ADDR.reg = ((uint32_t)(((slave_address) << 1U) | 1U));

    return master_read_bytes(master, data, data_size);
}

platform_samd21g18a_i2c_status_t
platform_samd21g18a_i2c_write_read (
    platform_samd21g18a_i2c_master_t       *master,
    platform_samd21g18a_i2c_slave_address_t slave_address,
    uint8_t const                          *write_data,
    size_t                                  write_data_size,
    uint8_t                                *read_data,
    size_t                                  read_data_size)
{
    platform_samd21g18a_i2c_status_t status;

    PLATFORM_SAMD21G18A_ASSERT(slave_address <= MAX_ADDRESS);
    PLATFORM_SAMD21G18A_ASSERT((write_data != NULL) || (write_data_size == 0U));
    PLATFORM_SAMD21G18A_ASSERT((read_data != NULL) || (read_data_size == 0U));

    if (read_data_size == 0U)
    {
        return platform_samd21g18a_i2c_write(
            master, slave_address, write_data, write_data_size);
    }

    // First phase: START and REPEATED START.
    master->I2CM.ADDR.reg = (uint32_t)((slave_address) << 1U);

    status = master_poll_until_ready(master);

    if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
    {
        master_send_stop(master);
        return status;
    }

    for (size_t index = 0U; index < write_data_size; index++)
    {
        master->I2CM.DATA.reg = write_data[index];

        status = master_poll_until_ready(master);

        if (status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
        {
            master_send_stop(master);
            return status;
        }
    }

    // Second phase: REPEATED START and STOP.
    master->I2CM.ADDR.reg = ((uint32_t)(((slave_address) << 1U) | 1U));

    return master_read_bytes(master, read_data, read_data_size);
}
