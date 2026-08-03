#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep

void
utils_gclk_poll_sync (void)
{
    while (GCLK->STATUS.bit.SYNCBUSY)
    {
    }

    return;
}

void
utils_gclk0_enable (uint16_t id)
{
    GCLK->CLKCTRL.reg
        = (uint16_t)(id | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN);

    return;
}
