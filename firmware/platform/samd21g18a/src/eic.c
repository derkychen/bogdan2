#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EIC_LINE_COUNT (16u)

static platform_samd21g18a_eic_callback_t eic_callbacks[EIC_LINE_COUNT];

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
    for (uint8_t index = 0u; index < EIC_LINE_COUNT; index++)
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
platform_samd21g18a_eic_pin_init (platform_samd21g18a_eic_pin_t *pin,
                                  uint8_t                        port_group,
                                  uint8_t                        number,
                                  uint8_t  peripheral_function,
                                  uint8_t  eic_line,
                                  uint32_t sense)
{
    uint8_t  config_index;
    uint8_t  bit_position;
    uint32_t mask;
    uint32_t value;
    uint8_t  pmux_index;

    PLATFORM_SAMD21G18A_ASSERT(pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(pin->port_group <= 1u);
    PLATFORM_SAMD21G18A_ASSERT(pin->number <= 31u);
    PLATFORM_SAMD21G18A_ASSERT(pin->eic_line < EIC_LINE_COUNT);

    pin->port_group          = port_group;
    pin->number              = number;
    pin->peripheral_function = peripheral_function;
    pin->eic_line            = eic_line;

    // Configure pin to act as EIC input.
    PORT->Group[pin->port_group].PINCFG[pin->number].reg
        = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;

    pmux_index = pin->number / 2u;

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

    // Configure pin sense.
    EIC->CTRL.bit.ENABLE = 0u;
    eic_poll_sync();

    config_index = pin->eic_line / 8u;
    bit_position = (uint8_t)((pin->eic_line % 8u) * 4u);

    mask  = 0xful << bit_position;
    value = sense << bit_position;

    EIC->CONFIG[config_index].reg
        = (EIC->CONFIG[config_index].reg & ~mask) | value;

    EIC->INTFLAG.reg = (1ul << pin->eic_line);

    EIC->CTRL.bit.ENABLE = 1u;
    eic_poll_sync();

    return;
}

void
platform_samd21g18a_eic_register_callback (
    const platform_samd21g18a_eic_pin_t *pin,
    platform_samd21g18a_eic_callback_t   callback)
{
    uint32_t primask;

    PLATFORM_SAMD21G18A_ASSERT(pin->eic_line < EIC_LINE_COUNT);

    primask = __get_PRIMASK();
    __disable_irq();

    eic_callbacks[pin->eic_line] = callback;

    if (primask == 0u)
    {
        __enable_irq();
    }

    return;
}

void
platform_samd21g18a_eic_line_enable (uint8_t eic_line)
{
    PLATFORM_SAMD21G18A_ASSERT(pin->eic_line < EIC_LINE_COUNT);

    EIC->INTFLAG.reg  = (1ul << eic_line);
    EIC->INTENSET.reg = (1ul << eic_line);

    return;
}

void
platform_samd21g18a_eic_line_disable (uint8_t eic_line)
{
    PLATFORM_SAMD21G18A_ASSERT(pin->eic_line < EIC_LINE_COUNT);

    EIC->INTENCLR.reg = (1ul << eic_line);

    return;
}

void
platform_samd21g18a_eic_line_clear (uint8_t eic_line)
{
    PLATFORM_SAMD21G18A_ASSERT(pin->eic_line < EIC_LINE_COUNT);

    EIC->INTFLAG.reg = (1ul << eic_line);

    return;
}

/** @brief Overrides the EIC_Handler function in the vector table. */
void
EIC_Handler (void)
{
    uint32_t                           flags;
    platform_samd21g18a_eic_callback_t callback;

    flags = EIC->INTFLAG.reg & EIC->INTENSET.reg;

    for (uint8_t eic_line = 0u; eic_line < EIC_LINE_COUNT; eic_line++)
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
