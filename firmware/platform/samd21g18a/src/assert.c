#include "platform/samd21g18a/assert.h"
#include "sam.h" // IWYU pragma: keep

void
platform_samd21g18a_assert_fail (char const *expression,
                                 char const *file,
                                 int         line)
{
    platform_samd21g18a_last_assert.expression = expression;
    platform_samd21g18a_last_assert.file       = file;
    platform_samd21g18a_last_assert.line       = line;

    __disable_irq();

    for (;;)
    {
        __BKPT(0);
    }
}
