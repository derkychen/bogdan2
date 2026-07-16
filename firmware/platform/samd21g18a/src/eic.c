#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/pin.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stddef.h>
#include <stdint.h>

#define EXTINT_LINE_COUNT (16U)

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

/** @brief Poll the EIC until it is ready. */
static inline void
eic_poll_sync (void)
{
    while ((EIC->STATUS.reg & EIC_STATUS_SYNCBUSY) != 0U)
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
    EIC->CTRL.bit.SWRST = 1U;
    eic_poll_sync();

    // Clear callback table.
    for (platform_samd21g18a_eic_extint_line_t line = 0U;
         line < EXTINT_LINE_COUNT;
         line++)
    {
        callback_entries[line].callback = NULL;
        callback_entries[line].context  = NULL;
    }

    // Enable EIC and allow CPU to receive interrupts.
    EIC->CTRL.bit.ENABLE = 1U;
    eic_poll_sync();

    NVIC_ClearPendingIRQ(EIC_IRQn);
    NVIC_SetPriority(EIC_IRQn, 1U);
    NVIC_EnableIRQ(EIC_IRQn);

    return;
}

void
platform_samd21g18a_eic_configure (platform_samd21g18a_eic_cfg_t const *cfg)
{
    PortGroup *port;
    uint8_t    pin_number;
    uint8_t    pmux_index;
    uint8_t    config_index;
    uint8_t    bit_position;
    uint32_t   pin_mask;
    uint32_t   line_mask;
    uint32_t   sense_mask;
    uint32_t   sense_value;

    PLATFORM_SAMD21G18A_ASSERT(cfg != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin->pin != NULL);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin->pin->port_group
                               < PLATFORM_SAMD21G18A_PIN_PORT_GROUP_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin->pin->number
                               < PLATFORM_SAMD21G18A_PIN_NUMBER_COUNT);
    PLATFORM_SAMD21G18A_ASSERT(cfg->eic_pin->line < EXTINT_LINE_COUNT);

    port = &PORT->Group[cfg->eic_pin->pin->port_group];

    pin_number = cfg->eic_pin->pin->number;
    pin_mask   = 1UL << pin_number;
    line_mask  = 1UL << cfg->eic_pin->line;

    // Disable the interrupt line and clear flags.
    EIC->INTENCLR.reg = line_mask;
    EIC->INTFLAG.reg  = line_mask;

    // Prevent contention with external output by setting the pin to an input
    // with high impedance.
    port->DIRCLR.reg             = pin_mask;
    port->PINCFG[pin_number].reg = 0U;

    // Configure the pin to the EIC peripheral function (A).
    pmux_index = pin_number / 2U;

    if ((pin_number & 1U) == 0U)
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
    config_index = cfg->eic_pin->line / 8U;
    bit_position = (uint8_t)((cfg->eic_pin->line % 8U) * 4U);

    sense_mask  = 0x7UL << bit_position;
    sense_value = ((uint32_t)cfg->sense & 0x7UL) << bit_position;

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
    uint32_t primask;

    PLATFORM_SAMD21G18A_ASSERT(line < EXTINT_LINE_COUNT);

    primask = __get_PRIMASK();
    __disable_irq();

    callback_entries[line] = (platform_samd21g18a_eic_callback_entry_t) {
        .callback = callback,
        .context  = context,
    };

    if (primask == 0U)
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

    EIC->INTENCLR.reg = (1UL << line);

    return;
}

void
platform_samd21g18a_eic_line_enable (platform_samd21g18a_eic_extint_line_t line)
{
    PLATFORM_SAMD21G18A_ASSERT(line < EXTINT_LINE_COUNT);

    EIC->INTFLAG.reg  = (1UL << line);
    EIC->INTENSET.reg = (1UL << line);

    return;
}

/** @brief Overrides the EIC_Handler function in the vector table. */
void
EIC_Handler (void)
{
    uint32_t                           flags;
    platform_samd21g18a_eic_callback_t callback;
    void                              *context;

    flags = EIC->INTFLAG.reg & EIC->INTENSET.reg;

    for (platform_samd21g18a_eic_extint_line_t line = 0U;
         line < EXTINT_LINE_COUNT;
         line++)
    {
        if ((flags & (1UL << line)) != 0U)
        {
            EIC->INTFLAG.reg = (1UL << line);
            callback         = callback_entries[line].callback;
            context          = callback_entries[line].context;

            if (callback != NULL)
            {
                callback(line, context);
            }
        }
    }

    return;
}
