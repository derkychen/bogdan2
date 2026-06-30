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

/** @brief Poll SERCOM1 I2C master until synchronized with a chosen mask. */
static inline void
poll_sync_mask (uint32_t mask)
{
    while ((SERCOM1->I2CM.SYNCBUSY.reg & mask) != 0u)
    {
    }
}

/** @brief Clear and set an I2C command. */
static inline void
set_command (uint32_t command)
{
    SERCOM1->I2CM.CTRLB.reg
        = (SERCOM1->I2CM.CTRLB.reg & ~SERCOM_I2CM_CTRLB_CMD_Msk)
          | SERCOM_I2CM_CTRLB_CMD(command);

    poll_sync_mask(SERCOM_I2CM_SYNCBUSY_SYSOP);
}

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

/** @brief Poll until the master is on the bus (ready for next command/byte). */
static platform_samd21g18a_i2c_result_t
poll_master_on_bus (void)
{
    uint32_t timeout = TIMEOUT_COUNT;

    while ((SERCOM1->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_MB) == 0u)
    {
        if ((SERCOM1->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0u)
        {
            return PLATFORM_SAMD21G18A_I2C_RESULT_ERROR;
        }

        if (timeout == 0u)
        {
            return PLATFORM_SAMD21G18A_I2C_RESULT_TIMEOUT;
        }

        timeout--;
    }

    if ((SERCOM1->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK) != 0u)
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_NACK;
    }

    return PLATFORM_SAMD21G18A_I2C_RESULT_OK;
}

/** @brief Poll until the slave is on the bus (ready for send byte). */
static platform_samd21g18a_i2c_result_t
poll_slave_on_bus (void)
{
    uint32_t timeout = TIMEOUT_COUNT;

    while ((SERCOM1->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_SB) == 0u)
    {
        if ((SERCOM1->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0u)
        {
            return PLATFORM_SAMD21G18A_I2C_RESULT_ERROR;
        }

        if (timeout == 0u)
        {
            return PLATFORM_SAMD21G18A_I2C_RESULT_TIMEOUT;
        }

        timeout--;
    }

    return PLATFORM_SAMD21G18A_I2C_RESULT_OK;
}

/** @brief Terminate the message. */
static void
send_stop (void)
{
    set_command(COMMAND_STOP);

    poll_sync_mask(SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

/** @brief Configure an I2C pin. */
static void
pin_configure (platform_samd21g18a_i2c_pin_t const *pin)
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

static platform_samd21g18a_i2c_result_t
read_bytes (uint8_t *data, size_t data_size)
{
    platform_samd21g18a_i2c_result_t result;
    size_t                           index;

    if (data_size == 0u)
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_OK;
    }

    SERCOM1->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
    poll_sync_mask(SERCOM_I2CM_SYNCBUSY_SYSOP);

    for (index = 0u; index < data_size; index++)
    {
        result = poll_slave_on_bus();

        if (result != PLATFORM_SAMD21G18A_I2C_RESULT_OK)
        {
            send_stop();
            return result;
        }

        if (index == (data_size - 1u))
        {
            SERCOM1->I2CM.CTRLB.reg |= SERCOM_I2CM_CTRLB_ACKACT;
        }
        else
        {
            SERCOM1->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
        }

        data[index] = (uint8_t)SERCOM1->I2CM.DATA.reg;

        if (index == (data_size - 1u))
        {
            set_command(COMMAND_STOP);
        }
        else
        {
            set_command(COMMAND_CONTINUE);
        }
    }

    poll_sync_mask(SERCOM_I2CM_SYNCBUSY_SYSOP);

    return PLATFORM_SAMD21G18A_I2C_RESULT_OK;
}

void
platform_samd21g18a_i2c_init (void)
{
    PM->APBCMASK.reg |= PM_APBCMASK_SERCOM1;

    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_SERCOM1_CORE | GCLK_CLKCTRL_GEN_GCLK0
                        | GCLK_CLKCTRL_CLKEN;

    platform_samd21g18a_utils_gclk_poll_sync();

    return;
}

void
platform_samd21g18a_i2c_bus_configure (platform_samd21g18a_i2c_bus_t const *bus)
{
    uint32_t frequency_hz;
    uint32_t rise_nsec;
    baud_t   baud;

    PLATFORM_SAMD21G18A_ASSERT(bus != NULL);
    PLATFORM_SAMD21G18A_ASSERT(bus->sda != NULL);
    PLATFORM_SAMD21G18A_ASSERT(bus->scl != NULL);

    frequency_hz = SCL_FREQUENCY_HZ;
    rise_nsec    = SCL_RISE_NSEC;

    baud_calculate(&baud, SystemCoreClock, frequency_hz, rise_nsec);

    pin_configure(bus->sda);
    pin_configure(bus->scl);

    SERCOM1->I2CM.CTRLA.bit.SWRST = 1u;

    poll_sync_mask(SERCOM_I2CM_SYNCBUSY_SWRST);

    SERCOM1->I2CM.CTRLA.bit.ENABLE = 0u;

    poll_sync_mask(SERCOM_I2CM_SYNCBUSY_ENABLE);

    SERCOM1->I2CM.CTRLA.reg
        = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_SDAHOLD(2u);

    // Enable smart mode for handling ACK behaviour after DATA reads.
    SERCOM1->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;

    poll_sync_mask(SERCOM_I2CM_SYNCBUSY_SYSOP);

    SERCOM1->I2CM.BAUD.bit.BAUD    = baud.baud;
    SERCOM1->I2CM.BAUD.bit.BAUDLOW = baud.baudlow;

    SERCOM1->I2CM.CTRLA.bit.ENABLE = 1u;

    poll_sync_mask(SERCOM_I2CM_SYNCBUSY_ENABLE);

    SERCOM1->I2CM.STATUS.bit.BUSSTATE = 1u;

    poll_sync_mask(SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

platform_samd21g18a_i2c_result_t
platform_samd21g18a_i2c_write (uint8_t        address,
                               uint8_t const *data,
                               size_t         data_size)
{
    platform_samd21g18a_i2c_result_t result;
    size_t                           index;

    PLATFORM_SAMD21G18A_ASSERT(address <= MAX_ADDRESS);

    if (address > MAX_ADDRESS)
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_INVALID_ARGUMENT;
    }

    if ((data == NULL) && (data_size != 0u))
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_INVALID_ARGUMENT;
    }

    SERCOM1->I2CM.ADDR.reg = (uint32_t)((address) << 1u);

    result = poll_master_on_bus();

    if (result != PLATFORM_SAMD21G18A_I2C_RESULT_OK)
    {
        send_stop();
        return result;
    }

    for (index = 0u; index < data_size; index++)
    {
        SERCOM1->I2CM.DATA.reg = data[index];

        result = poll_master_on_bus();

        if (result != PLATFORM_SAMD21G18A_I2C_RESULT_OK)
        {
            send_stop();
            return result;
        }
    }

    send_stop();

    return PLATFORM_SAMD21G18A_I2C_RESULT_OK;
}

platform_samd21g18a_i2c_result_t
platform_samd21g18a_i2c_read (uint8_t address, uint8_t *data, size_t data_size)
{
    PLATFORM_SAMD21G18A_ASSERT(address <= MAX_ADDRESS);

    if (address > MAX_ADDRESS)
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_INVALID_ARGUMENT;
    }

    if ((data == NULL) && (data_size != 0u))
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_INVALID_ARGUMENT;
    }

    if (data_size == 0u)
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_OK;
    }

    SERCOM1->I2CM.ADDR.reg = ((uint32_t)(((address) << 1u) | 1u));

    return read_bytes(data, data_size);
}

platform_samd21g18a_i2c_result_t
platform_samd21g18a_i2c_write_read (uint8_t        address,
                                    uint8_t const *write_data,
                                    size_t         write_size,
                                    uint8_t       *read_data,
                                    size_t         read_size)
{
    platform_samd21g18a_i2c_result_t result;
    size_t                           index;

    PLATFORM_SAMD21G18A_ASSERT(address <= MAX_ADDRESS);

    if (address > MAX_ADDRESS)
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_INVALID_ARGUMENT;
    }

    if ((write_data == NULL) && (write_size != 0u))
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_INVALID_ARGUMENT;
    }

    if ((read_data == NULL) && (read_size != 0u))
    {
        return PLATFORM_SAMD21G18A_I2C_RESULT_INVALID_ARGUMENT;
    }

    if (read_size == 0u)
    {
        return platform_samd21g18a_i2c_write(address, write_data, write_size);
    }

    /*
     * First phase:
     *   START + address(write) + write bytes
     *
     * No STOP at the end. The read address below creates a repeated START.
     */
    SERCOM1->I2CM.ADDR.reg = (uint32_t)((address) << 1u);

    result = poll_master_on_bus();

    if (result != PLATFORM_SAMD21G18A_I2C_RESULT_OK)
    {
        send_stop();
        return result;
    }

    for (index = 0u; index < write_size; index++)
    {
        SERCOM1->I2CM.DATA.reg = write_data[index];

        result = poll_master_on_bus();

        if (result != PLATFORM_SAMD21G18A_I2C_RESULT_OK)
        {
            send_stop();
            return result;
        }
    }

    /*
     * Second phase:
     *   repeated START + address(read) + read bytes + STOP
     */
    SERCOM1->I2CM.ADDR.reg = ((uint32_t)(((address) << 1u) | 1u));

    return read_bytes(read_data, read_size);
}
