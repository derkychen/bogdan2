/**
 * @file i2c.c
 * @brief Implementation of I2C functionality.
 */
#include "platform/samd21g18a/i2c.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/pin.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCL_LATENCY_CYCLES (10u)
#define SCL_FREQUENCY_HZ   (I2C_SCL_FREQUENCY_STANDARD_HZ)
#define SCL_RISE_NS        (I2C_SCL_RISE_STANDARD_NS)
#define COMMAND_CONTINUE   (2u)
#define COMMAND_STOP       (3u)
#define TIMEOUT_COUNT      (100000u)
#define MAX_ADDRESS        (0x7Fu)

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
    pin_port_group_t pin_port_group;

    /** Pin number. */
    pin_number_t pin_number;

    /** Peripheral function for I2C. */
    pin_peripheral_function_t pin_peripheral_function;

    /** I2C master. */
    i2c_master_t master;

    /** SERCOM pad. */
    i2c_sercom_pad_t pad;
} route_t;

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

static route_t const routes[] = {
    {
        .pin_port_group          = 0u,
        .pin_number              = 16u,
        .pin_peripheral_function = PIN_PERIPHERAL_FUNCTION_C,
        .master                  = I2C_MASTER_SERCOM1,
        .pad                     = I2C_SERCOM_PAD0,
    },
    {
        .pin_port_group          = 0u,
        .pin_number              = 17u,
        .pin_peripheral_function = PIN_PERIPHERAL_FUNCTION_C,
        .master                  = I2C_MASTER_SERCOM1,
        .pad                     = I2C_SERCOM_PAD1,
    },
};

static master_data_t const master_data[I2C_SERCOM_COUNT] = {
    [I2C_MASTER_SERCOM0] = {
        .sercom    = SERCOM0,
        .apbc_mask = PM_APBCMASK_SERCOM0,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM0_CORE,
    },
    [I2C_MASTER_SERCOM1] = {
        .sercom    = SERCOM1,
        .apbc_mask = PM_APBCMASK_SERCOM1,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM1_CORE,
    },
    [I2C_MASTER_SERCOM2] = {
        .sercom    = SERCOM2,
        .apbc_mask = PM_APBCMASK_SERCOM2,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM2_CORE,
    },
    [I2C_MASTER_SERCOM3] = {
        .sercom    = SERCOM3,
        .apbc_mask = PM_APBCMASK_SERCOM3,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM3_CORE,
    },
    [I2C_MASTER_SERCOM4] = {
        .sercom    = SERCOM4,
        .apbc_mask = PM_APBCMASK_SERCOM4,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM4_CORE,
    },
    [I2C_MASTER_SERCOM5] = {
        .sercom    = SERCOM5,
        .apbc_mask = PM_APBCMASK_SERCOM5,
        .gclk_id   = GCLK_CLKCTRL_ID_SERCOM5_CORE,
    },
};

static inline uint32_t ceiling_divide(uint64_t numerator, uint32_t denominator);
static inline Sercom  *master_get_sercom(i2c_master_t master);
static void            baud_calculate(baud_t  *baud,
                                      uint32_t clock_frequency_hz,
                                      uint32_t scl_frequency_hz,
                                      uint32_t scl_rise_ns);
static inline void     sercom_poll_sync_mask(Sercom *sercom, uint32_t mask);
static inline void     sercom_set_command(Sercom *sercom, uint32_t command);
static i2c_status_t    sercom_poll_master_ready(Sercom *sercom);
static i2c_status_t    sercom_poll_slave_ready(Sercom *sercom);
static void            sercom_send_stop(Sercom *sercom);
static i2c_status_t    sercom_read_bytes(Sercom  *sercom,
                                         uint8_t *data,
                                         size_t   data_size);
static pin_peripheral_function_t pin_peripheral_function(
    i2c_pin_t const *i2c_pin, i2c_master_t master);
static void pin_configure(i2c_pin_t const *i2c_pin, i2c_master_t master);

void
i2c_configure (i2c_master_t     master,
               i2c_pin_t const *sda,
               i2c_pin_t const *scl,
               uint32_t         scl_frequency_hz,
               uint32_t         scl_rise_ns)
{
    ASSERT(master < I2C_SERCOM_COUNT);
    ASSERT(sda != NULL);
    ASSERT(scl != NULL);

    uint32_t frequency_hz = scl_frequency_hz;
    uint32_t rise_ns      = scl_rise_ns;

    baud_t baud;
    baud_calculate(&baud, SystemCoreClock, frequency_hz, rise_ns);

    utils_apbc_enable(master_data[master].apbc_mask);
    utils_gclk0_enable(master_data[master].gclk_id);

    pin_configure(sda, master);
    pin_configure(scl, master);

    Sercom *sercom = master_get_sercom(master);

    sercom->I2CM.CTRLA.bit.SWRST = 1u;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SWRST);

    sercom->I2CM.CTRLA.bit.ENABLE = 0u;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_ENABLE);

    sercom->I2CM.CTRLA.reg
        = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_SDAHOLD(2u);

    // Enable smart mode for handling ACK behaviour after DATA reads.
    sercom->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    sercom->I2CM.BAUD.bit.BAUD    = baud.baud;
    sercom->I2CM.BAUD.bit.BAUDLOW = baud.baudlow;

    sercom->I2CM.CTRLA.bit.ENABLE = 1u;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_ENABLE);

    sercom->I2CM.STATUS.bit.BUSSTATE = 1u;

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

i2c_status_t
i2c_write (i2c_master_t        master,
           i2c_slave_address_t slave_address,
           uint8_t const      *data,
           size_t              data_size)
{
    ASSERT(slave_address <= MAX_ADDRESS);
    ASSERT((data != NULL) || (data_size == 0u));

    Sercom *sercom = master_get_sercom(master);

    sercom->I2CM.ADDR.reg = (uint32_t)((slave_address) << 1u);

    i2c_status_t status = sercom_poll_master_ready(sercom);

    if (status != I2C_STATUS_OK)
    {
        sercom_send_stop(sercom);
        return status;
    }

    for (size_t index = 0u; index < data_size; index++)
    {
        sercom->I2CM.DATA.reg = data[index];

        status = sercom_poll_master_ready(sercom);

        if (status != I2C_STATUS_OK)
        {
            sercom_send_stop(sercom);

            return status;
        }
    }

    sercom_send_stop(sercom);

    return I2C_STATUS_OK;
}

i2c_status_t
i2c_read (i2c_master_t        master,
          i2c_slave_address_t slave_address,
          uint8_t            *data,
          size_t              data_size)
{
    ASSERT(slave_address <= MAX_ADDRESS);
    ASSERT((data != NULL) || (data_size == 0u));

    if (data_size == 0u)
    {
        return I2C_STATUS_OK;
    }

    Sercom *sercom = master_get_sercom(master);

    sercom->I2CM.ADDR.reg
        = ((uint32_t)((((uint32_t)slave_address) << 1u) | 1u));

    return sercom_read_bytes(sercom, data, data_size);
}

i2c_status_t
i2c_write_read (i2c_master_t        master,
                i2c_slave_address_t slave_address,
                uint8_t const      *write_data,
                size_t              write_data_size,
                uint8_t            *read_data,
                size_t              read_data_size)
{
    ASSERT(slave_address <= MAX_ADDRESS);
    ASSERT((write_data != NULL) || (write_data_size == 0u));
    ASSERT((read_data != NULL) || (read_data_size == 0u));

    if (read_data_size == 0u)
    {
        return i2c_write(master, slave_address, write_data, write_data_size);
    }

    Sercom *sercom = master_get_sercom(master);

    // First phase: START and REPEATED START.
    sercom->I2CM.ADDR.reg = (uint32_t)((slave_address) << 1u);

    i2c_status_t status = sercom_poll_master_ready(sercom);

    if (status != I2C_STATUS_OK)
    {
        sercom_send_stop(sercom);
        return status;
    }

    for (size_t index = 0u; index < write_data_size; index++)
    {
        sercom->I2CM.DATA.reg = write_data[index];

        status = sercom_poll_master_ready(sercom);

        if (status != I2C_STATUS_OK)
        {
            sercom_send_stop(sercom);
            return status;
        }
    }

    // Second phase: REPEATED START and STOP.
    sercom->I2CM.ADDR.reg
        = ((uint32_t)((((uint32_t)slave_address) << 1u) | 1u));

    return sercom_read_bytes(sercom, read_data, read_data_size);
}

/** @brief Ceiling division for `uint32_t`. */
static inline uint32_t
ceiling_divide (uint64_t numerator, uint32_t denominator)
{
    return (uint32_t)((numerator + (uint64_t)denominator - 1u)
                      / (uint64_t)denominator);
}

/** @brief Get the SERCOM registers for an I2C master. */
static inline Sercom *
master_get_sercom (i2c_master_t master)
{
    return master_data[master].sercom;
}

/** @brief Calculate BAUD and BAUDLOW. */
static void
baud_calculate (baud_t  *baud,
                uint32_t clock_frequency_hz,
                uint32_t scl_frequency_hz,
                uint32_t scl_rise_ns)
{
    ASSERT(baud != NULL);
    ASSERT((clock_frequency_hz != 0u) && (scl_frequency_hz != 0u));

    uint32_t scl_period_cycles
        = ceiling_divide(clock_frequency_hz, scl_frequency_hz);
    uint32_t scl_rise_cycles = ceiling_divide(
        (uint64_t)clock_frequency_hz * (uint64_t)scl_rise_ns, 1000000000u);

    ASSERT(scl_period_cycles > (scl_rise_cycles + SCL_LATENCY_CYCLES));

    uint32_t programmable_cycles
        = scl_period_cycles - scl_rise_cycles - SCL_LATENCY_CYCLES;

    ASSERT(programmable_cycles <= 510u);

    baud->baud    = (uint8_t)(programmable_cycles / 2u);
    baud->baudlow = (uint8_t)(programmable_cycles - baud->baud);

    return;
}

/** @brief Poll SERCOM I2C master until synchronized with a chosen mask. */
static inline void
sercom_poll_sync_mask (Sercom *sercom, uint32_t mask)
{
    ASSERT(sercom != NULL);

    while ((sercom->I2CM.SYNCBUSY.reg & mask) != 0u)
    {
    }

    return;
}

/** @brief Clear and set an I2C command. */
static inline void
sercom_set_command (Sercom *sercom, uint32_t command)
{
    ASSERT(sercom != NULL);

    sercom->I2CM.CTRLB.reg
        = (sercom->I2CM.CTRLB.reg & ~SERCOM_I2CM_CTRLB_CMD_Msk)
          | SERCOM_I2CM_CTRLB_CMD(command);

    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

/** @brief Poll until the master is on the bus (ready for next command/byte). */
static i2c_status_t
sercom_poll_master_ready (Sercom *sercom)
{
    ASSERT(sercom != NULL);

    uint32_t timeout = TIMEOUT_COUNT;

    while ((sercom->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_MB) == 0u)
    {
        if ((sercom->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0u)
        {
            return I2C_STATUS_ERR_BUS;
        }

        if (timeout == 0u)
        {
            return I2C_STATUS_ERR_TIMEOUT;
        }

        timeout--;
    }

    if ((sercom->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_RXNACK) != 0u)
    {
        return I2C_STATUS_ERR_NACK;
    }

    return I2C_STATUS_OK;
}

/** @brief Poll until the slave is on the bus (ready for send byte). */
static i2c_status_t
sercom_poll_slave_ready (Sercom *sercom)
{
    ASSERT(sercom != NULL);

    uint32_t timeout = TIMEOUT_COUNT;

    while ((sercom->I2CM.INTFLAG.reg & SERCOM_I2CM_INTFLAG_SB) == 0u)
    {
        if ((sercom->I2CM.STATUS.reg & SERCOM_I2CM_STATUS_BUSERR) != 0u)
        {
            return I2C_STATUS_ERR_BUS;
        }

        if (timeout == 0u)
        {
            return I2C_STATUS_ERR_TIMEOUT;
        }

        timeout--;
    }

    return I2C_STATUS_OK;
}

/** @brief Terminate the message. */
static void
sercom_send_stop (Sercom *sercom)
{
    ASSERT(sercom != NULL);

    sercom_set_command(sercom, COMMAND_STOP);
    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    return;
}

static i2c_status_t
sercom_read_bytes (Sercom *sercom, uint8_t *data, size_t data_size)
{
    ASSERT(sercom != NULL);

    if (data_size == 0u)
    {
        return I2C_STATUS_OK;
    }

    sercom->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT;
    sercom_poll_sync_mask(sercom, SERCOM_I2CM_SYNCBUSY_SYSOP);

    i2c_status_t status;

    for (size_t index = 0u; index < data_size; index++)
    {
        status = sercom_poll_slave_ready(sercom);

        if (status != I2C_STATUS_OK)
        {
            sercom_send_stop(sercom);
            return status;
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

    return I2C_STATUS_OK;
}

/** @brief Resolve an I2C pin route to a PORT peripheral function. */
static pin_peripheral_function_t
pin_peripheral_function (i2c_pin_t const *i2c_pin, i2c_master_t master)
{
    ASSERT(i2c_pin != NULL);
    ASSERT(i2c_pin->pin != NULL);

    for (size_t i = 0u; i < sizeof(routes) / sizeof(routes[0]); i++)
    {
        if ((routes[i].pin_port_group == i2c_pin->pin->port_group)
            && (routes[i].pin_number == i2c_pin->pin->number)
            && (routes[i].master == master) && (routes[i].pad == i2c_pin->pad))
        {
            return routes[i].pin_peripheral_function;
        }
    }

    ASSERT(false);

    return PIN_PERIPHERAL_FUNCTION_A;
}

/** @brief Configure an I2C pin. */
static void
pin_configure (i2c_pin_t const *i2c_pin, i2c_master_t master)
{
    ASSERT(i2c_pin != NULL);
    ASSERT(i2c_pin->pin != NULL);
    ASSERT(pin_port_group_valid(i2c_pin->pin->port_group));
    ASSERT(pin_number_valid(i2c_pin->pin->number));

    pin_peripheral_function_t peripheral_function
        = pin_peripheral_function(i2c_pin, master);

    // NOTE: No pull-up resistor is used because the IND.I/O board has them
    //       built in.
    pin_set_cfg(i2c_pin->pin, true, true, false, false);

    pin_set_peripheral_function(i2c_pin->pin, peripheral_function);

    return;
}
