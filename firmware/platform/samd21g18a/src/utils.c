/**
 * @file utils.c
 * @brief Implementation of miscellaneous utility functions.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep

void
utils_gclk0_enable (uint16_t id)
{
    GCLK->CLKCTRL.reg
        = (uint16_t)(id | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN);

    while (GCLK->STATUS.bit.SYNCBUSY)
    {
    }

    return;
}

void
utils_apbc_enable (uint32_t mask)
{
    PM->APBCMASK.reg |= mask;

    return;
}
