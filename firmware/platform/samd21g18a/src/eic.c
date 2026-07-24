#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/pin.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stddef.h>
#include <stdint.h>

#define EXTINT_LINE_COUNT (16u)

static uint8_t const sense_shifts[PLATFORM_SAMD21G18A_EIC_SENSE_COUNT] = {
    [PLATFORM_SAMD21G18A_EIC_SENSE_RISE] = EIC_CONFIG_SENSE0_RISE_Val,
    [PLATFORM_SAMD21G18A_EIC_SENSE_FALL] = EIC_CONFIG_SENSE0_FALL_Val,
    [PLATFORM_SAMD21G18A_EIC_SENSE_BOTH] = EIC_CONFIG_SENSE0_BOTH_Val,
    [PLATFORM_SAMD21G18A_EIC_SENSE_HIGH] = EIC_CONFIG_SENSE0_HIGH_Val,
    [PLATFORM_SAMD21G18A_EIC_SENSE_LOW]  = EIC_CONFIG_SENSE0_LOW_Val,
};

/** @brief EIC callback entry format. */
typedef struct
{
    /** The callback that runs on an interrupt. */
    platform_samd21g18a_eic_callback_t callback;

    /** Context pointer passed to the callback. */
    void *context;
} platform_samd21g18a_eic_callback_entry_t;

static platform_samd21g18a_eic_callback_entry_t
    callback_entries[EXTINT_LINE_COUNT];

static inline void eic_poll_sync(void);

void
platform_samd21g18a_eic_init (void)
{
    // Enable EIC clock and set its source to GCLK0.
    PM->APBAMASK.reg |= PM_APBAMASK_EIC;

    GCLK->CLKCTRL.reg
        = GCLK_CLKCTRL_ID_EIC | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;

    platform_samd21g18a_utils_gclk_poll_sync();

    // Reset EIC.
    EIC->CTRL.bit.SWRST = 1u;
    eic_poll_sync();

    // Clear callback table.
    for (platform_samd21g18a_eic_extint_line_t line = 0u;
         line < EXTINT_LINE_COUNT;
         line++)
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
platform_samd21g18a_eic_configure (platform_samd21g18a_eic_cfg_t const *cfg)
{
    PLATFORM_SAMD21G18A_ASSERT(cfg != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin->pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin->pin->port_group
                               < PLATFORM_SAMD21G18A_PIN_PORT_GROUP_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin->pin->number
                               < PLATFORM_SAMD21G18A_PIN_NUMBER_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin->line < EXTINT_LINE_COUNT);

    PortGroup *port = &PORT->Group[cfg->eic_pin->pin->port_group];

    uint8_t  pin_number = cfg->eic_pin->pin->number;
    uint32_t pin_mask   = 1u << pin_number;
    uint32_t line_mask  = 1u << cfg->eic_pin->line;

    // Disable the interrupt line and clear flags.
    EIC->INTENCLR.reg = line_mask;
    EIC->INTFLAG.reg  = line_mask;

    // Prevent contention with external output by setting the pin to an input
    // with high impedance.
    port->DIRCLR.reg             = pin_mask;
    port->PINCFG[pin_number].reg = 0u;

    // Configure the pin to the EIC peripheral function (A).
    uint8_t pmux_index = pin_number / 2u;

    if ((pin_number & 1u) == 0u)
    {
        port->PMUX[pmux_index].bit.PMUXE
            = PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_A;
    }
    else
    {
        port->PMUX[pmux_index].bit.PMUXO
            = PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_A;
    }

    port->PINCFG[pin_number].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;

    // Configure the pin's sense.
    uint8_t config_index = cfg->eic_pin->line / 8u;
    uint8_t bit_position = (uint8_t)((cfg->eic_pin->line % 8u) * 4u);

    uint32_t sense_mask  = 0x7u << bit_position;
    uint32_t sense_value = ((uint32_t)sense_shifts[cfg->sense] & 0x7u)
                           << bit_position;

    EIC->CONFIG[config_index].reg
        = (EIC->CONFIG[config_index].reg & ~sense_mask) | sense_value;

    // Clear flags.
    EIC->INTFLAG.reg = line_mask;

    return;
}

void
platform_samd21g18a_eic_register_callback_entry (
    platform_samd21g18a_eic_extint_line_t line,
    platform_samd21g18a_eic_callback_t    callback,
    void                                 *context)
{
    PLATFORM_SAMD21G18A_ASSERT(line < EXTINT_LINE_COUNT);

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    callback_entries[line] = (platform_samd21g18a_eic_callback_entry_t) {
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
platform_samd21g18a_eic_line_disable (
    platform_samd21g18a_eic_extint_line_t line)
{
    PLATFORM_SAMD21G18A_ASSERT(line < EXTINT_LINE_COUNT);

    EIC->INTENCLR.reg = (1u << line);

    return;
}

void
platform_samd21g18a_eic_line_enable (platform_samd21g18a_eic_extint_line_t line)
{
    PLATFORM_SAMD21G18A_ASSERT(line < EXTINT_LINE_COUNT);

    EIC->INTFLAG.reg  = (1u << line);
    EIC->INTENSET.reg = (1u << line);

    return;
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
        platform_samd21g18a_eic_extint_line_t const line
            = (platform_samd21g18a_eic_extint_line_t)__builtin_ctz(pending);

        platform_samd21g18a_eic_callback_entry_t const entry
            = callback_entries[line];

        // Remove least-significant set bit.
        pending &= pending - 1u;

        PLATFORM_SAMD21G18A_ASSERT(entry.callback != NULL);
        entry.callback(line, entry.context);
    }

    return;
}

/** @brief Poll the EIC until it is ready. */
static inline void
eic_poll_sync (void)
{
    while ((EIC->STATUS.reg & EIC_STATUS_SYNCBUSY) != 0u)
    {
    }

    return;
}
