#include "platform/samd21g18a/utils.h"
#include "samd21g18a.h"

inline void
platform_samd21g18a_poll_gclk_until_synchronized (void)
{
    while (GCLK->STATUS.bit.SYNCBUSY)
    {
    }

    return;
}
