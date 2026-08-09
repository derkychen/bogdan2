/**
 * @file eic.c
 * @brief Implementation of EIC functionality.
 *
 * This implementation stores an internal array of callbacks. When an external
 * interrupt line fires, it passes the `context` pointer into the callback,
 * enabling those who register interrupts to pass context, allowing for
 * interrupts to access the data they are supposed to.
 */
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/evsys.h"
#include "platform/samd21g18a/pin.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stddef.h>
#include <stdint.h>

#define EXTINT_LINE_COUNT (16u)

static uint16_t const sense_values[EIC_SENSE_COUNT] = {
    [EIC_SENSE_NONE] = EIC_CONFIG_SENSE0_NONE_Val,
    [EIC_SENSE_RISE] = EIC_CONFIG_SENSE0_RISE_Val,
    [EIC_SENSE_FALL] = EIC_CONFIG_SENSE0_FALL_Val,
    [EIC_SENSE_BOTH] = EIC_CONFIG_SENSE0_BOTH_Val,
    [EIC_SENSE_HIGH] = EIC_CONFIG_SENSE0_HIGH_Val,
    [EIC_SENSE_LOW]  = EIC_CONFIG_SENSE0_LOW_Val,
};

/** @brief EIC callback entry format. */
typedef struct
{
    eic_callback_t callback; /**< The callback that runs on an interrupt. */
    void          *context;  /**< Context pointer passed to the callback. */
} eic_callback_entry_t;

static eic_callback_entry_t callback_entries[EXTINT_LINE_COUNT];

bool
eic_extint_line_valid (eic_extint_line_t line)
{
    return line < EXTINT_LINE_COUNT;
}

bool
eic_sense_valid (eic_sense_t sense)
{
    return sense < EIC_SENSE_COUNT;
}

void
eic_poll_sync (void)
{
    while ((EIC->STATUS.reg & EIC_STATUS_SYNCBUSY) != 0u)
    {
    }

    return;
}

void
eic_init (void)
{
    // Enable EIC clock and set its source to GCLK0.
    PM->APBAMASK.reg |= PM_APBAMASK_EIC;

    utils_gclk0_enable(GCLK_CLKCTRL_ID_EIC);

    // Reset EIC.
    EIC->CTRL.bit.SWRST = 1u;
    eic_poll_sync();

    // Clear callback table.
    for (eic_extint_line_t line = 0u; line < EXTINT_LINE_COUNT; line++)
    {
        callback_entries[line].callback = NULL;
        callback_entries[line].context  = NULL;
    }

    // Enable EIC and allow CPU to receive interrupts.
    EIC->CTRL.bit.ENABLE = 1u;
    eic_poll_sync();

    NVIC_ClearPendingIRQ(EIC_IRQn);
    NVIC_SetPriority(EIC_IRQn, 1u);
    NVIC_EnableIRQ(EIC_IRQn);

    return;
}

void
eic_configure (eic_pin_t const *eic_pin, eic_sense_t sense)
{
    ASSERT(eic_pin != NULL);
    ASSERT(eic_pin->pin != NULL);
    ASSERT(pin_port_group_valid(eic_pin->pin->port_group));
    ASSERT(pin_number_valid(eic_pin->pin->number));
    ASSERT(eic_extint_line_valid(eic_pin->line));
    ASSERT(eic_sense_valid(sense));

    // Disable the interrupt line and clear flags.
    uint32_t line_mask = 1u << eic_pin->line;
    bool     enabled   = EIC->INTENSET.reg & line_mask;

    EIC->INTENCLR.reg = line_mask;
    EIC->INTFLAG.reg  = line_mask;

    // Prevent contention with external output by setting the pin to an input
    // with high impedance.
    PortGroup *port       = &PORT->Group[eic_pin->pin->port_group];
    uint8_t    pin_number = eic_pin->pin->number;
    uint32_t   pin_mask   = 1u << pin_number;

    port->DIRCLR.reg = pin_mask;

    // Set the pin to an input with peripheral function to A (EIC).
    pin_set_cfg(eic_pin->pin, true, true, false, false);
    pin_set_peripheral_function(eic_pin->pin, PIN_PERIPHERAL_FUNCTION_A);

    // Configure the pin's sense.
    uint8_t config_index = eic_pin->line / 8u;
    uint8_t bit_position = (uint8_t)((eic_pin->line % 8u) * 4u);

    uint32_t sense_mask  = 0x7u << bit_position;
    uint32_t sense_value = ((uint32_t)sense_values[sense] & 0x7u)
                           << bit_position;

    EIC->CONFIG[config_index].reg
        = (EIC->CONFIG[config_index].reg & ~sense_mask) | sense_value;

    // Clear flags.
    EIC->INTFLAG.reg = line_mask;

    // Enable the line if it was previously disabled.
    if (enabled)
    {
        EIC->INTENSET.reg = line_mask;
    }

    return;
}

void
eic_register_callback_entry (eic_extint_line_t line,
                             eic_callback_t    callback,
                             void             *context)
{
    ASSERT(eic_extint_line_valid(line));

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    callback_entries[line] = (eic_callback_entry_t) {
        .callback = callback,
        .context  = context,
    };

    if (primask == 0u)
    {
        __enable_irq();
    }

    return;
}

void
eic_interrupt_disable (eic_extint_line_t line)
{
    ASSERT(eic_extint_line_valid(line));

    EIC->INTENCLR.reg = (1u << line);

    return;
}

void
eic_interrupt_enable (eic_extint_line_t line)
{
    ASSERT(eic_extint_line_valid(line));

    EIC->INTFLAG.reg  = (1u << line);
    EIC->INTENSET.reg = (1u << line);

    return;
}

void
eic_event_enable (eic_extint_line_t line)
{
    ASSERT(eic_extint_line_valid(line));

    EIC->EVCTRL.reg |= (1u << line);

    return;
}

void
eic_event_disable (eic_extint_line_t line)
{
    ASSERT(eic_extint_line_valid(line));

    EIC->EVCTRL.reg &= ~(1u << line);

    return;
}

evsys_generator_t
eic_event_generator (eic_extint_line_t line)
{
    ASSERT(eic_extint_line_valid(line));

    return (evsys_generator_t)(EVSYS_ID_GEN_EIC_EXTINT_0 + (uint32_t)line);
}

/** @brief Overrides the EIC_Handler function in the vector table. */
void
EIC_Handler (void)
{
    uint32_t pending
        = EIC->INTFLAG.reg & EIC->INTENSET.reg & EIC_INTFLAG_EXTINT_Msk;

    // NOTE: This clears all flags so new interrupts can be received while
    //       callbacks execute.
    EIC->INTFLAG.reg = pending;

    while (pending != 0u)
    {
        eic_extint_line_t const line
            = (eic_extint_line_t)__builtin_ctz(pending);

        eic_callback_entry_t const entry = callback_entries[line];

        // Remove least-significant set bit.
        pending &= pending - 1u;

        ASSERT(entry.callback != NULL);
        entry.callback(line, entry.context);
    }

    return;
}
