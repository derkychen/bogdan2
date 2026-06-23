#ifndef PLATFORM_SAMD21G18A_UTILS_H
#define PLATFORM_SAMD21G18A_UTILS_H

#include "samd21g18a.h"

/** @brief Poll the GCLK register until it is synchronized. */
static inline void
platform_samd21g18a_poll_gclk_until_synchronized (void)
{

    while (GCLK->STATUS.bit.SYNCBUSY)
    {
    }

    return;
}

#endif
