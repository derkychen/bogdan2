#include "platform/samd21g18a/eic.h"
#include "samd21g18a.h"
#include <stddef.h>

#define PLATFORM_SAMD21G18A_EIC_LINE_COUNT (16u)

typedef struct
{
    platform_samd21g18a_eic_callback_t callback;
    void                              *context;
} eic_callback_entry_t;

static eic_callback_entry_t eic_callbacks[PLATFORM_SAMD21G18A_EIC_LINE_COUNT];

static inline void
eic_poll_sync (void)
{
    while ((EIC->STATUS.reg & EIC_STATUS_SYNCBUSY) != 0u)
    {
    }
}

static uint8_t
eic_sense_to_sense_bits (platform_samd21g18a_eic_sense_t sense)
{
    switch (sense)
    {
        case PLATFORM_SAMD21G18A_EIC_SENSE_RISE: {
            return EIC_CONFIG_SENSE0_RISE_Val;
        }

        case PLATFORM_SAMD21G18A_EIC_SENSE_FALL: {
            return EIC_CONFIG_SENSE0_FALL_Val;
        }

        case PLATFORM_SAMD21G18A_EIC_SENSE_BOTH: {
            return EIC_CONFIG_SENSE0_BOTH_Val;
        }

        case PLATFORM_SAMD21G18A_EIC_SENSE_HIGH: {
            return EIC_CONFIG_SENSE0_HIGH_Val;
        }

        case PLATFORM_SAMD21G18A_EIC_SENSE_LOW: {
            return EIC_CONFIG_SENSE0_LOW_Val;
        }

        default: {
            return EIC_CONFIG_SENSE0_RISE_Val;
        }
    }
}

void
platform_samd21g18a_eic_init (void)
{
    uint8_t index;

    PM->APBAMASK.reg |= PM_APBAMASK_EIC;

    GCLK->CLKCTRL.reg
        = GCLK_CLKCTRL_ID_EIC | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;
    eic_poll_sync();

    EIC->CTRL.bit.SWRST = 1u;
    eic_poll_sync();

    for (index = 0u; index < PLATFORM_SAMD21G18A_EIC_LINE_COUNT; index++)
    {
        eic_callbacks[index].callback = NULL;
        eic_callbacks[index].context  = NULL;
    }

    EIC->CTRL.bit.ENABLE = 1u;

    NVIC_ClearPendingIRQ(EIC_IRQn);
    NVIC_SetPriority(EIC_IRQn, 1u);
    NVIC_EnableIRQ(EIC_IRQn);

    return;
}

void
platform_samd21g18a_eic_pin_config_sense (
    const platform_samd21g18a_pin_eic_t *pin,
    platform_samd21g18a_eic_sense_t      sense)
{
    uint8_t  config_index;
    uint8_t  bit_position;
    uint32_t mask;
    uint32_t value;
    uint8_t  pmux_index;

    // Configure pin mux to EIC peripheral function.
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

    // Configure EIC sense.
    config_index = pin->extint_line / 8u;
    bit_position = (uint8_t)((pin->extint_line % 8u) * 4u);

    mask  = 0xful << bit_position;
    value = ((uint32_t)eic_sense_to_sense_bits(sense)) << bit_position;

    EIC->CONFIG[config_index].reg
        = (EIC->CONFIG[config_index].reg & ~mask) | value;

    EIC->INTFLAG.reg = (1ul << pin->extint_line);

    return;
}

bool
platform_samd21g18a_eic_register_callback (
    uint8_t                            extint_line,
    platform_samd21g18a_eic_callback_t callback,
    void                              *context)
{
    if (extint_line >= PLATFORM_SAMD21G18A_EIC_LINE_COUNT)
    {
        return false;
    }

    __disable_irq();

    eic_callbacks[extint_line].callback = callback;
    eic_callbacks[extint_line].context  = context;

    __enable_irq();

    return true;
}

void
platform_samd21g18a_eic_enable_line (uint8_t extint_line)
{
    if (extint_line >= PLATFORM_SAMD21G18A_EIC_LINE_COUNT)
    {
        return;
    }

    EIC->INTENSET.reg = (1ul << extint_line);

    return;
}

void
platform_samd21g18a_eic_disable_line (uint8_t extint_line)
{
    if (extint_line >= PLATFORM_SAMD21G18A_EIC_LINE_COUNT)
    {
        return;
    }

    EIC->INTENCLR.reg = (1ul << extint_line);

    return;
}

/** @brief Overrides the `USB_Handler` function in the vector table. */
void
EIC_Handler (void)
{
    uint32_t                           flags;
    uint8_t                            line;
    platform_samd21g18a_eic_callback_t callback;
    void                              *context;

    flags = EIC->INTFLAG.reg & EIC->INTENSET.reg;

    for (line = 0u; line < PLATFORM_SAMD21G18A_EIC_LINE_COUNT; line++)
    {
        if ((flags & (1ul << line)) != 0u)
        {
            EIC->INTFLAG.reg = (1ul << line);

            callback = eic_callbacks[line].callback;
            context  = eic_callbacks[line].context;

            if (callback != NULL)
            {
                callback(line, context);
            }
        }
    }

    return;
}
