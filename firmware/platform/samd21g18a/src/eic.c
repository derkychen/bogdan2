#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EXTINT_LINE_COUNT (16u)

static platform_samd21g18a_eic_callback_t eic_callbacks[EXTINT_LINE_COUNT];

/** @brief Poll the EIC until it is ready. */
static inline void
eic_poll_sync (void)
{
    while ((EIC->STATUS.reg & EIC_STATUS_SYNCBUSY) != 0u)
    {
    }

    return;
}

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
    for (uint8_t index = 0u; index < EXTINT_LINE_COUNT; index++)
    {
        eic_callbacks[index] = NULL;
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
platform_samd21g18a_eic_pin_configure (platform_samd21g18a_eic_cfg_t const *cfg)
{
    uint8_t  config_index;
    uint8_t  bit_position;
    uint32_t mask;
    uint32_t value;
    uint8_t  pmux_index;

    PLATFORM_SAMD21G18A_ASSERT(cfg != NULL);

    PLATFORM_SAMD21G18A_ASSERT(cfg->line != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->pin != NULL);

    PLATFORM_SAMD21G18A_ASSERT(cfg->line->extint_line < EXTINT_LINE_COUNT);

    PLATFORM_SAMD21G18A_ASSERT(cfg->pin->port_group <= 1u);
    PLATFORM_SAMD21G18A_ASSERT(cfg->pin->number <= 31u);

    // Configure pin to act as EIC input.
    PORT->Group[cfg->pin->port_group].PINCFG[cfg->pin->number].reg
        = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;

    pmux_index = cfg->pin->number / 2u;

    if ((cfg->pin->number & 1u) == 0u)
    {
        PORT->Group[cfg->pin->port_group].PMUX[pmux_index].bit.PMUXE
            = cfg->pin->peripheral_function;
    }
    else
    {
        PORT->Group[cfg->pin->port_group].PMUX[pmux_index].bit.PMUXO
            = cfg->pin->peripheral_function;
    }

    // Configure pin sense.
    EIC->CTRL.bit.ENABLE = 0u;
    eic_poll_sync();

    config_index = cfg->line->extint_line / 8u;
    bit_position = (uint8_t)((cfg->line->extint_line % 8u) * 4u);

    mask  = 0xful << bit_position;
    value = cfg->sense << bit_position;

    EIC->CONFIG[config_index].reg
        = (EIC->CONFIG[config_index].reg & ~mask) | value;

    EIC->INTFLAG.reg = (1ul << cfg->line->extint_line);

    EIC->CTRL.bit.ENABLE = 1u;
    eic_poll_sync();

    return;
}

void
platform_samd21g18a_eic_register_callback (
    platform_samd21g18a_eic_line_t const *line,
    platform_samd21g18a_eic_callback_t    callback)
{
    uint32_t primask;

    PLATFORM_SAMD21G18A_ASSERT(eic_line < EXTINT_LINE_COUNT);

    primask = __get_PRIMASK();
    __disable_irq();

    eic_callbacks[line->extint_line] = callback;

    if (primask == 0u)
    {
        __enable_irq();
    }

    return;
}

void
platform_samd21g18a_eic_line_enable (platform_samd21g18a_eic_line_t const *line)
{
    PLATFORM_SAMD21G18A_ASSERT(line->extint_line < EXTINT_LINE_COUNT);

    EIC->INTFLAG.reg  = (1ul << line->extint_line);
    EIC->INTENSET.reg = (1ul << line->extint_line);

    return;
}

void
platform_samd21g18a_eic_line_disable (
    platform_samd21g18a_eic_line_t const *line)
{
    PLATFORM_SAMD21G18A_ASSERT(line->extint_line < EXTINT_LINE_COUNT);

    EIC->INTENCLR.reg = (1ul << line->extint_line);

    return;
}

void
platform_samd21g18a_eic_line_clear (platform_samd21g18a_eic_line_t const *line)
{
    PLATFORM_SAMD21G18A_ASSERT(line->extint_line < EXTINT_LINE_COUNT);

    EIC->INTFLAG.reg = (1ul << line->extint_line);

    return;
}

/** @brief Overrides the EIC_Handler function in the vector table. */
void
EIC_Handler (void)
{
    uint32_t                           flags;
    platform_samd21g18a_eic_callback_t callback;

    flags = EIC->INTFLAG.reg & EIC->INTENSET.reg;

    for (uint8_t eic_line = 0u; eic_line < EXTINT_LINE_COUNT; eic_line++)
    {
        if ((flags & (1ul << eic_line)) != 0u)
        {
            EIC->INTFLAG.reg = (1ul << eic_line);
            callback         = eic_callbacks[eic_line];

            if (callback != NULL)
            {
                callback(eic_line);
            }
        }
    }

    return;
}
