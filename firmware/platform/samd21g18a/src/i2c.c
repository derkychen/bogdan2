#include "platform/samd21g18a/i2c.h"
#include "platform/samd21g18a/utils.h"
#include "platform/samd21g18a/assert.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCL_LATENCY_CYCLES (10u)
#define SCL_FREQUENCY_HZ   (PLATFORM_SAMD21G18A_I2C_STANDARD_SCL_FREQUENCY_HZ)
#define SCL_RISE_NSEC      (PLATFORM_SAMD21G18A_I2C_STANDARD_SCL_RISE_NSEC)

#define COMMAND_CONTINUE (2u)
#define COMMAND_STOP     (3u)

#define TIMEOUT_COUNT (100000u)

#define MAX_ADDRESS (0x7fu)

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
    return (uint32_t)((numerator + (uint64_t)denominator - 1u)
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
    PLATFORM_SAMD21G18A_ASSERT((clock_frequency_hz != 0u)
                               && (scl_frequency_hz != 0u));

    scl_period_cycles = ceiling_divide(clock_frequency_hz, scl_frequency_hz);
    scl_rise_cycles   = ceiling_divide(
        (uint64_t)clock_frequency_hz * (uint64_t)scl_rise_ns, 1000000000u);

    PLATFORM_SAMD21G18A_ASSERT(scl_period_cycles
                               > (scl_rise_cycles + SCL_LATENCY_CYCLES));

    programmable_cycles
        = scl_period_cycles - scl_rise_cycles - SCL_LATENCY_CYCLES;

    PLATFORM_SAMD21G18A_ASSERT(programmable_cycles <= 510u);

    baud->baud    = (uint8_t)(programmable_cycles / 2u);
    baud->baudlow = (uint8_t)(programmable_cycles - baud->baud);

    return;
}

/** @brief Poll SERCOM I2C master until synchronized with a chosen mask. */
static inline void
sercom_poll_sync_mask (Sercom *sercom, uint32_t mask)
{
    while ((sercom->I2CM.SYNCBUSY.reg & mask) != 0u)
    {
    }

    return;
}

/** @brief Enable the clock of a SERCOM. */
static void
sercom_enable_clock (Sercom *sercom)
{
    PLATFORM_SAMD21G18A_ASSERT(sercom == SERCOM1);

    if (sercom == SERCOM1)
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
sercom_set_command (Sercom *sercom, uint32_t command)
{
    sercom->I2CM.CTRLB.reg
        = (sercom->I2CM.CTRLB.reg & ~SERCOM_I2CM_CTRLB_CMD_Msk)
          | SERCOM_I2CM_CTRLB_CMD(command);

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);
}

/** @brief Poll until the master is on the bus (ready for next command/byte). */
static platform_samd21g18a_i2c_status_code_t
sercom_poll_for_master (Sercom *sercom)
{
    uint32_t timeout = TIMEOUT_COUNT;

    while ((sercom->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_MB) == 0u)
    {
        if ((sercom->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0u)
        {
            return PLATFORM_SAMD21G18A_I2C_ERR;
        }

        if (timeout == 0u)
        {
            return PLATFORM_SAMD21G18A_I2C_TIMEOUT;
        }

        timeout--;
    }

    if ((sercom->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK) != 0u)
    {
        return PLATFORM_SAMD21G18A_I2C_NACK;
    }

    return PLATFORM_SAMD21G18A_I2C_OK;
}

/** @brief Poll until the slave is on the bus (ready for send byte). */
static platform_samd21g18a_i2c_status_code_t
sercom_poll_for_slave (Sercom *sercom)
{
    uint32_t timeout = TIMEOUT_COUNT;

    while ((sercom->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_SB) == 0u)
    {
        if ((sercom->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0u)
        {
            return PLATFORM_SAMD21G18A_I2C_ERR;
        }

        if (timeout == 0u)
        {
            return PLATFORM_SAMD21G18A_I2C_TIMEOUT;
        }

        timeout--;
    }

    return PLATFORM_SAMD21G18A_I2C_OK;
}

/** @brief Terminate the message. */
static void
sercom_send_stop (Sercom *sercom)
{
    sercom_set_command(sercom, COMMAND_STOP);

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

static platform_samd21g18a_i2c_status_code_t
sercom_read_bytes (Sercom *sercom, uint8_t *data, size_t data_size)
{
    platform_samd21g18a_i2c_status_code_t status_code;
    size_t                                index;

    if (data_size == 0u)
    {
        return PLATFORM_SAMD21G18A_I2C_OK;
    }

    sercom->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    for (index = 0u; index < data_size; index++)
    {
        status_code = sercom_poll_for_slave(sercom);

        if (status_code != PLATFORM_SAMD21G18A_I2C_OK)
        {
            sercom_send_stop(sercom);
            return status_code;
        }

        if (index == (data_size - 1u))
        {
            sercom->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_ACKACT;
        }
        else
        {
            sercom->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
        }

        data[index] = (uint8_t)sercom->I2CM.DATA.reg;

        if (index == (data_size - 1u))
        {
            sercom_set_command(sercom, COMMAND_STOP);
        }
        else
        {
            sercom_set_command(sercom, COMMAND_CONTINUE);
        }
    }

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return PLATFORM_SAMD21G18A_I2C_OK;
}

/** @brief Configure an I2C pin. */
static void
pin_configure (platform_samd21g18a_pin_t const *pin)
{
    uint8_t pmux_index;

    PLATFORM_SAMD21G18A_ASSERT(pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(pin->port_group <= 1u);
    PLATFORM_SAMD21G18A_ASSERT(pin->number <= 31u);
    PLATFORM_SAMD21G18A_ASSERT(pin->peripheral_function <= 7u);

    pmux_index = pin->number / 2u;

    // NOTE: No pull-up resistor is used because the IND.I/O board has them
    //       built in.
    PORT->Group[pin->port_group].PINCFG[pin->number].reg
        = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;

    if ((pin->number & 1u) == 0u)
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
    PLATFORM_SAMD21G18A_ASSERT(cfg->sercom != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->sda != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->scl != NULL);

    frequency_hz = SCL_FREQUENCY_HZ;
    rise_nsec    = SCL_RISE_NSEC;

    baud_calculate(&baud, SystemCoreClock, frequency_hz, rise_nsec);

    sercom_enable_clock(cfg->sercom);

    pin_configure(cfg->sda);
    pin_configure(cfg->scl);

    cfg->sercom->I2CM.CTRLA.bit.SWRST = 1u;

    sercom_poll_sync_mask(cfg->sercom, SERCOM_I2CM_SYNCBUSY_SWRST);

    cfg->sercom->I2CM.CTRLA.bit.ENABLE = 0u;

    sercom_poll_sync_mask(cfg->sercom, SERCOM_I2CM_SYNCBUSY_ENABLE);

    cfg->sercom->I2CM.CTRLA.reg
        = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_SDAHOLD(2u);

    // Enable smart mode for handling ACK behaviour after DATA reads.
    cfg->sercom->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;

    sercom_poll_sync_mask(cfg->sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    cfg->sercom->I2CM.BAUD.bit.BAUD    = baud.baud;
    cfg->sercom->I2CM.BAUD.bit.BAUDLOW = baud.baudlow;

    cfg->sercom->I2CM.CTRLA.bit.ENABLE = 1u;

    sercom_poll_sync_mask(cfg->sercom, SERCOM_I2CM_SYNCBUSY_ENABLE);

    cfg->sercom->I2CM.STATUS.bit.BUSSTATE = 1u;

    sercom_poll_sync_mask(cfg->sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

platform_samd21g18a_i2c_status_code_t
platform_samd21g18a_i2c_write (Sercom        *sercom,
                               uint8_t        address,
                               uint8_t const *data,
                               size_t         data_size)
{
    platform_samd21g18a_i2c_status_code_t status_code;
    size_t                                index;

    PLATFORM_SAMD21G18A_ASSERT(address <= MAX_ADDRESS);

    if (address > MAX_ADDRESS)
    {
        return PLATFORM_SAMD21G18A_I2C_INVALID_ARGUMENT;
    }

    if ((data == NULL) && (data_size != 0u))
    {
        return PLATFORM_SAMD21G18A_I2C_INVALID_ARGUMENT;
    }

    sercom->I2CM.ADDR.reg = (uint32_t)((address) << 1u);

    status_code = sercom_poll_for_master(sercom);

    if (status_code != PLATFORM_SAMD21G18A_I2C_OK)
    {
        sercom_send_stop(sercom);
        return status_code;
    }

    for (index = 0u; index < data_size; index++)
    {
        sercom->I2CM.DATA.reg = data[index];

        status_code = sercom_poll_for_master(sercom);

        if (status_code != PLATFORM_SAMD21G18A_I2C_OK)
        {
            sercom_send_stop(sercom);
            return status_code;
        }
    }

    sercom_send_stop(sercom);

    return PLATFORM_SAMD21G18A_I2C_OK;
}

platform_samd21g18a_i2c_status_code_t
platform_samd21g18a_i2c_read (Sercom  *sercom,
                              uint8_t  address,
                              uint8_t *data,
                              size_t   data_size)
{
    PLATFORM_SAMD21G18A_ASSERT(address <= MAX_ADDRESS);

    if (address > MAX_ADDRESS)
    {
        return PLATFORM_SAMD21G18A_I2C_INVALID_ARGUMENT;
    }

    if ((data == NULL) && (data_size != 0u))
    {
        return PLATFORM_SAMD21G18A_I2C_INVALID_ARGUMENT;
    }

    if (data_size == 0u)
    {
        return PLATFORM_SAMD21G18A_I2C_OK;
    }

    sercom->I2CM.ADDR.reg = ((uint32_t)(((address) << 1u) | 1u));

    return sercom_read_bytes(sercom, data, data_size);
}

platform_samd21g18a_i2c_status_code_t
platform_samd21g18a_i2c_write_read (Sercom        *sercom,
                                    uint8_t        address,
                                    uint8_t const *write_data,
                                    size_t         write_size,
                                    uint8_t       *read_data,
                                    size_t         read_size)
{
    platform_samd21g18a_i2c_status_code_t status_code;
    size_t                                index;

    PLATFORM_SAMD21G18A_ASSERT(address <= MAX_ADDRESS);

    if (address > MAX_ADDRESS)
    {
        return PLATFORM_SAMD21G18A_I2C_INVALID_ARGUMENT;
    }

    if ((write_data == NULL) && (write_size != 0u))
    {
        return PLATFORM_SAMD21G18A_I2C_INVALID_ARGUMENT;
    }

    if ((read_data == NULL) && (read_size != 0u))
    {
        return PLATFORM_SAMD21G18A_I2C_INVALID_ARGUMENT;
    }

    if (read_size == 0u)
    {
        return platform_samd21g18a_i2c_write(
            sercom, address, write_data, write_size);
    }

    // First phase: START and REPEATED START.
    sercom->I2CM.ADDR.reg = (uint32_t)((address) << 1u);

    status_code = sercom_poll_for_master(sercom);

    if (status_code != PLATFORM_SAMD21G18A_I2C_OK)
    {
        sercom_send_stop(sercom);
        return status_code;
    }

    for (index = 0u; index < write_size; index++)
    {
        sercom->I2CM.DATA.reg = write_data[index];

        status_code = sercom_poll_for_master(sercom);

        if (status_code != PLATFORM_SAMD21G18A_I2C_OK)
        {
            sercom_send_stop(sercom);
            return status_code;
        }
    }

    // Second phase: REPEATED START and STOP.
    sercom->I2CM.ADDR.reg = ((uint32_t)(((address) << 1u) | 1u));

    return sercom_read_bytes(sercom, read_data, read_size);
}
