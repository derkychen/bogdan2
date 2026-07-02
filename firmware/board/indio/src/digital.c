#include "board/indio/digital.h"
#include "drivers/pca9555.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/i2c.h"
#include <stddef.h>
#include <stdint.h>

// Digital expander interrupt registers specific to the IND.I/O.
#define INDIO_DIGITAL_EXPANDER_REG_LATCH            (0x44U)
#define INDIO_DIGITAL_EXPANDER_REG_MASK             (0x4AU)
#define INDIO_DIGITAL_EXPANDER_REG_INTERRUPT_SOURCE (0x4CU)

// External interrupt line of the SAMD21 that the IND.I/O digital expander is
// connected to.
#define INDIO_DIGITAL_EXPANDER_EXTINT_LINE (8U)

#define CALLBACK_ENTRIES_COUNT ((size_t)BOARD_INDIO_DIGITAL_PIN_COUNT)

#define MASK_ALL \
    ((board_indio_digital_mask_t)((1U << BOARD_INDIO_DIGITAL_PIN_COUNT) - 1U))

/** @brief IND.I/O expander register type. */
typedef uint8_t indio_expander_reg_t;

/** @brief Internal interrupt callback entry structure. */
typedef struct
{
    board_indio_digital_mask_t                    mask;
    board_indio_digital_mask_interrupt_callback_t callback;
    void                                         *context;
} board_indio_digital_interrupt_callback_entry_t;

// Number of pending interrupts.
static uint32_t volatile interrupt_pending_count;

/** @brief The PCA9555 instance used by the IND.I/O for digital I/O. */
static drivers_pca9555_device_t const board_indio_digital_expander = {
    .master  = SERCOM1,
    .address = 0x21U,
};

// Shadow pin states.
static drivers_pca9555_outputs_t    outputs_shadow;
static drivers_pca9555_inputs_t     inputs_shadow;
static drivers_pca9555_cfgs_t       cfgs_shadow;
static drivers_pca9555_polarities_t polarities_shadow;

// Shadow IND.I/O interrupt configuration.
static board_indio_digital_interrupt_cfg_t interrupt_cfg_shadow;

// Callback entries.
static board_indio_digital_interrupt_callback_entry_t
    callback_entries[CALLBACK_ENTRIES_COUNT];

/** @brief Output pin bit position from pin. */
static inline uint8_t
output_bit (board_indio_digital_pin_t pin)
{
    PLATFORM_SAMD21G18A_ASSERT(pin < BOARD_INDIO_DIGITAL_PIN_COUNT);

    return (uint8_t)(((uint8_t)pin * 2U) + 1U);
}

/** @brief Input pin bit position from pin. */
static inline uint8_t
input_bit (board_indio_digital_pin_t pin)
{
    PLATFORM_SAMD21G18A_ASSERT(pin < BOARD_INDIO_DIGITAL_PIN_COUNT);

    return (uint8_t)((uint8_t)pin * 2U);
}

/** @brief Output pin mask from pin. */
static inline uint16_t
output_mask_raw (board_indio_digital_pin_t pin)
{
    return (uint16_t)(1U << output_bit(pin));
}

/** @brief Input pin mask from pin. */
static inline uint16_t
input_mask_raw (board_indio_digital_pin_t pin)
{
    return (uint16_t)(1U << input_bit(pin));
}

/** @brief Call callbacks for a corresponding digital pin mask. */
static void
dispatch_callback_entries (board_indio_digital_mask_t fired_mask)
{
    for (size_t index = 0u; index < CALLBACK_ENTRIES_COUNT; index++)
    {
        board_indio_digital_mask_t callback_fired_mask;

        if (callback_entries[index].callback == NULL)
        {
            continue;
        }

        callback_fired_mask = fired_mask & callback_entries[index].mask;

        if (callback_fired_mask == 0u)
        {
            continue;
        }

        callback_entries[index].callback(callback_fired_mask,
                                         callback_entries[index].context);
    }
}

/** @brief Get the raw input mask from the digital pin mask. */
static uint16_t
logical_mask_to_raw_input_mask (board_indio_digital_mask_t logical_mask)
{
    uint16_t raw_mask;

    PLATFORM_SAMD21G18A_ASSERT(
        (logical_mask & (board_indio_digital_mask_t)~MASK_ALL) == 0U);

    raw_mask = 0U;

    for (board_indio_digital_pin_t pin = BOARD_INDIO_DIGITAL_PIN_CH1;
         pin < BOARD_INDIO_DIGITAL_PIN_COUNT;
         pin++)
    {
        if ((logical_mask & (board_indio_digital_mask_t)(1U << pin)) != 0U)
        {
            raw_mask |= input_mask_raw(pin);
        }
    }

    return raw_mask;
}

/** @brief Convert a PCA9555 status code to a digital status code. */
static board_indio_digital_status_t
pca9555_status_to_digital_status (drivers_pca9555_status_t status)
{
    switch (status)
    {
        case DRIVERS_PCA9555_STATUS_OK:
            return BOARD_INDIO_DIGITAL_STATUS_OK;
        case DRIVERS_PCA9555_STATUS_I2C_ERR:
        default:
            return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }
}

/** @brief Convert the raw input mask to a digital pin mask. */
static board_indio_digital_mask_t
raw_input_mask_to_logical_mask (uint16_t raw_mask)
{
    board_indio_digital_mask_t logical_mask;

    logical_mask = 0U;

    for (board_indio_digital_pin_t pin = BOARD_INDIO_DIGITAL_PIN_CH1;
         pin < BOARD_INDIO_DIGITAL_PIN_COUNT;
         pin++)
    {
        if ((raw_mask & input_mask_raw(pin)) != 0U)
        {
            logical_mask |= (board_indio_digital_mask_t)(1U << pin);
        }
    }

    return logical_mask;
}

/** @brief Write to an IND.I/O expander register. */
static board_indio_digital_status_t
indio_expander_reg_write (uint8_t reg, uint16_t value)
{
    platform_samd21g18a_i2c_status_t i2c_status;
    uint8_t                          data[3];

    data[0] = reg;
    data[1] = (uint8_t)(value & 0xFFU);
    data[2] = (uint8_t)((value >> 8U) & 0xFFU);

    i2c_status
        = platform_samd21g18a_i2c_write(board_indio_digital_expander.master,
                                        board_indio_digital_expander.address,
                                        data,
                                        sizeof(data));

    if (i2c_status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
    {
        return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}

/** @brief Read from an IND.I/O expander register. */
static board_indio_digital_status_t
indio_expander_reg_read (uint8_t reg, uint16_t *value)
{
    platform_samd21g18a_i2c_status_t i2c_status;
    uint8_t                          data[2];

    PLATFORM_SAMD21G18A_ASSERT(value != NULL);

    i2c_status = platform_samd21g18a_i2c_write_read(
        board_indio_digital_expander.master,
        board_indio_digital_expander.address,
        &reg,
        1U,
        data,
        sizeof(data));

    if (i2c_status != PLATFORM_SAMD21G18A_I2C_STATUS_OK)
    {
        return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }

    *value = ((uint16_t)data[1] << 8U) | data[0];

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}

/** @brief Dispatches callbacks corresponding to read latched sources. */
static board_indio_digital_status_t
interrupt_source_service (board_indio_digital_mask_t service_mask)
{
    board_indio_digital_mask_t source_mask;

    PLATFORM_SAMD21G18A_ASSERT(
        (service_mask & (board_indio_digital_mask_t)~MASK_ALL) == 0u);

    if (board_indio_digital_interrupt_source_read(&source_mask)
        != BOARD_INDIO_DIGITAL_STATUS_OK)
    {
        return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }

    source_mask &= service_mask;

    if (source_mask != 0u)
    {
        dispatch_callback_entries(source_mask);
    }

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}

/**
 * @brief The EIC callback used by the IND.I/O expander.
 *
 * This function only increments the count of pending interrupts.
 */
static void
board_indio_digital_eic_callback (platform_samd21g18a_eic_extint_line_t line)
{
    (void)line;

    interrupt_pending_count++;

    return;
}

board_indio_digital_status_t
board_indio_digital_init (void)
{
    drivers_pca9555_status_t pca_status;

    // Default pin states.
    inputs_shadow     = 0u;
    outputs_shadow    = BOARD_INDIO_DIGITAL_DEFAULT_OUTPUTS;
    cfgs_shadow       = BOARD_INDIO_DIGITAL_DEFAULT_CFG;
    polarities_shadow = BOARD_INDIO_DIGITAL_DEFAULT_POLARITY;

    // Default interrupt configuration.
    interrupt_cfg_shadow.allowed_mask = 0u;
    interrupt_cfg_shadow.latched_mask = 0u;

    // No pending interrupts.
    interrupt_pending_count = 0u;

    // Clear callback entries.
    for (size_t index = 0u; index < CALLBACK_ENTRIES_COUNT; index++)
    {
        callback_entries[index].mask     = 0u;
        callback_entries[index].callback = NULL;
        callback_entries[index].context  = NULL;
    }

    // Write default outputs.
    //
    // NOTE: The output latches must be written before configuring any pins as
    // outputs to prevent glitches.
    pca_status = drivers_pca9555_write_outputs(&board_indio_digital_expander,
                                               outputs_shadow);

    if (pca_status != DRIVERS_PCA9555_STATUS_OK)
    {
        return pca9555_status_to_digital_status(pca_status);
    }

    // Write default polarities.
    pca_status = drivers_pca9555_write_polarities(&board_indio_digital_expander,
                                                  polarities_shadow);

    if (pca_status != DRIVERS_PCA9555_STATUS_OK)
    {
        return pca9555_status_to_digital_status(pca_status);
    }

    // Write default configurations.
    pca_status = drivers_pca9555_write_cfgs(&board_indio_digital_expander,
                                            cfgs_shadow);

    if (pca_status != DRIVERS_PCA9555_STATUS_OK)
    {
        return pca9555_status_to_digital_status(pca_status);
    }

    // Read in initial inputs.
    pca_status = drivers_pca9555_read_inputs(&board_indio_digital_expander,
                                             &inputs_shadow);

    if (pca_status != DRIVERS_PCA9555_STATUS_OK)
    {
        return pca9555_status_to_digital_status(pca_status);
    }

    // Disable and unlatch all interrupt sources.
    if (indio_expander_reg_write(INDIO_DIGITAL_EXPANDER_REG_MASK, 0u)
        != BOARD_INDIO_DIGITAL_STATUS_OK)
    {
        return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }

    if (indio_expander_reg_write(INDIO_DIGITAL_EXPANDER_REG_LATCH, 0u)
        != BOARD_INDIO_DIGITAL_STATUS_OK)
    {
        return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }

    platform_samd21g18a_eic_register_callback(
        INDIO_DIGITAL_EXPANDER_EXTINT_LINE, board_indio_digital_eic_callback);

    // Drain stale source state during startup.
    //
    // This is safe before any real operation is active.
    return board_indio_digital_interrupt_source_drain(MASK_ALL);
}

void
board_indio_digital_interrupt_pin_register_callback (
    board_indio_digital_mask_t                    mask,
    board_indio_digital_mask_interrupt_callback_t callback,
    void                                         *context)
{
    PLATFORM_SAMD21G18A_ASSERT((mask & (board_indio_digital_mask_t)~MASK_ALL)
                               == 0u);

    if (callback == NULL)
    {
        for (size_t index = 0u; index < CALLBACK_ENTRIES_COUNT; index++)
        {
            if (callback_entries[index].mask == mask)
            {
                callback_entries[index].mask     = 0u;
                callback_entries[index].callback = NULL;
                callback_entries[index].context  = NULL;
            }
        }

        return;
    }

    // Replace an existing entry with the same mask to watch.
    for (size_t index = 0u; index < CALLBACK_ENTRIES_COUNT; index++)
    {
        if (callback_entries[index].mask == mask)
        {
            callback_entries[index].callback = callback;
            callback_entries[index].context  = context;
            return;
        }
    }

    // Use the first free entry slot.
    for (size_t index = 0u; index < CALLBACK_ENTRIES_COUNT; index++)
    {
        if (callback_entries[index].callback == NULL)
        {
            callback_entries[index].mask     = mask;
            callback_entries[index].callback = callback;
            callback_entries[index].context  = context;
            return;
        }
    }

    // The caller should not register more entries than can be handled.
    PLATFORM_SAMD21G18A_ASSERT(0);
}

board_indio_digital_status_t
board_indio_digital_task (void)
{
    uint32_t pending_count;

    if (interrupt_pending_count == 0u)
    {
        return BOARD_INDIO_DIGITAL_STATUS_OK;
    }

    pending_count           = interrupt_pending_count;
    interrupt_pending_count = 0u;

    (void)pending_count;

    return interrupt_source_service(interrupt_cfg_shadow.allowed_mask);
}

board_indio_digital_status_t
board_indio_digital_interrupt_configure (
    board_indio_digital_interrupt_cfg_t const *cfg)
{
    uint16_t raw_allowed_mask;
    uint16_t raw_latched_mask;

    PLATFORM_SAMD21G18A_ASSERT(cfg != NULL);
    PLATFORM_SAMD21G18A_ASSERT(
        (cfg->allowed_mask & (board_indio_digital_mask_t)~MASK_ALL) == 0u);
    PLATFORM_SAMD21G18A_ASSERT(
        (cfg->latched_mask & (board_indio_digital_mask_t)~MASK_ALL) == 0u);

    raw_allowed_mask = logical_mask_to_raw_input_mask(cfg->allowed_mask);
    raw_latched_mask = logical_mask_to_raw_input_mask(cfg->latched_mask);

    /*
     * Configure latch before enabling the mask so short events are captured
     * according to the new phase behavior.
     */
    if (indio_expander_reg_write(INDIO_DIGITAL_EXPANDER_REG_LATCH,
                                 raw_latched_mask)
        != BOARD_INDIO_DIGITAL_STATUS_OK)
    {
        return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }

    if (indio_expander_reg_write(INDIO_DIGITAL_EXPANDER_REG_MASK,
                                 raw_allowed_mask)
        != BOARD_INDIO_DIGITAL_STATUS_OK)
    {
        return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }

    interrupt_cfg_shadow = *cfg;

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}

board_indio_digital_status_t
board_indio_digital_interrupt_source_read (board_indio_digital_mask_t *mask)
{
    uint16_t raw_source;

    PLATFORM_SAMD21G18A_ASSERT(mask != NULL);

    if (indio_expander_reg_read(INDIO_DIGITAL_EXPANDER_REG_INTERRUPT_SOURCE,
                                &raw_source)
        != BOARD_INDIO_DIGITAL_STATUS_OK)
    {
        return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }

    *mask = raw_input_mask_to_logical_mask(raw_source);

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}

board_indio_digital_status_t
board_indio_digital_interrupt_source_drain (board_indio_digital_mask_t mask)
{
    board_indio_digital_mask_t source_mask;
    board_indio_digital_mask_t unexpected_mask;

    PLATFORM_SAMD21G18A_ASSERT((mask & (board_indio_digital_mask_t)~MASK_ALL)
                               == 0u);

    if (board_indio_digital_interrupt_source_read(&source_mask)
        != BOARD_INDIO_DIGITAL_STATUS_OK)
    {
        return BOARD_INDIO_DIGITAL_STATUS_ERR;
    }

    // Dispatches anything outside of the requested drain mask in case of
    // hardware automatically clearing all source bits on a read.
    unexpected_mask = source_mask & (board_indio_digital_mask_t)~mask;

    if (unexpected_mask != 0u)
    {
        dispatch_callback_entries(unexpected_mask);
    }

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}

board_indio_digital_status_t
board_indio_digital_pin_set_cfg (board_indio_digital_pin_t     pin,
                                 board_indio_digital_pin_cfg_t cfg)
{
    drivers_pca9555_status_t pca_status;
    uint16_t                 next_cfgs;
    uint16_t                 raw_input_mask;
    uint16_t                 raw_output_mask;

    PLATFORM_SAMD21G18A_ASSERT(pin < BOARD_INDIO_DIGITAL_PIN_COUNT);
    PLATFORM_SAMD21G18A_ASSERT((cfg == BOARD_INDIO_DIGITAL_CFG_INPUT)
                               || (cfg == BOARD_INDIO_DIGITAL_CFG_OUTPUT));

    raw_input_mask  = input_mask_raw(pin);
    raw_output_mask = output_mask_raw(pin);
    next_cfgs       = cfgs_shadow;

    if (cfg == BOARD_INDIO_DIGITAL_CFG_INPUT)
    {
        next_cfgs |= raw_input_mask;
        next_cfgs |= raw_output_mask;
    }
    else
    {
        next_cfgs |= raw_input_mask;
        next_cfgs &= (drivers_pca9555_cfgs_t)~raw_output_mask;
    }

    pca_status
        = drivers_pca9555_write_cfgs(&board_indio_digital_expander, next_cfgs);

    if (pca_status != DRIVERS_PCA9555_STATUS_OK)
    {
        return pca9555_status_to_digital_status(pca_status);
    }

    cfgs_shadow = next_cfgs;

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}

board_indio_digital_status_t
board_indio_digital_pin_set_polarity (
    board_indio_digital_pin_t pin, board_indio_digital_pin_polarity_t polarity)
{
    drivers_pca9555_status_t pca_status;
    uint16_t                 next_polarities;
    uint16_t                 raw_mask;

    PLATFORM_SAMD21G18A_ASSERT(pin < BOARD_INDIO_DIGITAL_PIN_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(
        (polarity == BOARD_INDIO_DIGITAL_POLARITY_NORMAL)
        || (polarity == BOARD_INDIO_DIGITAL_POLARITY_INVERTED));

    raw_mask        = input_mask_raw(pin);
    next_polarities = polarities_shadow;

    if (polarity == BOARD_INDIO_DIGITAL_POLARITY_INVERTED)
    {
        next_polarities |= raw_mask;
    }
    else
    {
        next_polarities &= (drivers_pca9555_polarities_t)~raw_mask;
    }

    pca_status = drivers_pca9555_write_polarities(&board_indio_digital_expander,
                                                  next_polarities);

    if (pca_status != DRIVERS_PCA9555_STATUS_OK)
    {
        return pca9555_status_to_digital_status(pca_status);
    }

    polarities_shadow = next_polarities;

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}

board_indio_digital_status_t
board_indio_digital_pin_write (board_indio_digital_pin_t       pin,
                               board_indio_digital_pin_level_t level)
{
    drivers_pca9555_status_t pca_status;
    uint16_t                 next_outputs;
    uint16_t                 raw_mask;

    PLATFORM_SAMD21G18A_ASSERT(pin < BOARD_INDIO_DIGITAL_PIN_COUNT);
    PLATFORM_SAMD21G18A_ASSERT((level == BOARD_INDIO_DIGITAL_LEVEL_LOW)
                               || (level == BOARD_INDIO_DIGITAL_LEVEL_HIGH));

    raw_mask     = output_mask_raw(pin);
    next_outputs = outputs_shadow;

    if (level == BOARD_INDIO_DIGITAL_LEVEL_HIGH)
    {
        next_outputs |= raw_mask;
    }
    else
    {
        next_outputs &= (uint16_t)~raw_mask;
    }

    pca_status = drivers_pca9555_write_outputs(&board_indio_digital_expander,
                                               next_outputs);

    if (pca_status != DRIVERS_PCA9555_STATUS_OK)
    {
        return pca9555_status_to_digital_status(pca_status);
    }

    outputs_shadow = next_outputs;

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}

board_indio_digital_status_t
board_indio_digital_pin_read (board_indio_digital_pin_t        pin,
                              board_indio_digital_pin_level_t *level)
{
    drivers_pca9555_status_t pca_status;
    uint16_t                 inputs;
    uint16_t                 raw_mask;

    PLATFORM_SAMD21G18A_ASSERT(pin < BOARD_INDIO_DIGITAL_PIN_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(level != NULL);

    pca_status
        = drivers_pca9555_read_inputs(&board_indio_digital_expander, &inputs);

    if (pca_status != DRIVERS_PCA9555_STATUS_OK)
    {
        return pca9555_status_to_digital_status(pca_status);
    }

    inputs_shadow = inputs;
    raw_mask      = input_mask_raw(pin);

    if ((inputs & raw_mask) != 0u)
    {
        *level = BOARD_INDIO_DIGITAL_LEVEL_HIGH;
    }
    else
    {
        *level = BOARD_INDIO_DIGITAL_LEVEL_LOW;
    }

    return BOARD_INDIO_DIGITAL_STATUS_OK;
}
