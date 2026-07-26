#include "platform/samd21g18a/assert.h"
#include "sam.h" // IWYU pragma: keep

platform_samd21g18a_assert_data_t volatile platform_samd21g18a_assert_data;

_Noreturn void
platform_samd21g18a_assert_fail (char const *expression,
                                 char const *file,
                                 int         line)
{
    platform_samd21g18a_assert_data.expression = expression;
    platform_samd21g18a_assert_data.file       = file;
    platform_samd21g18a_assert_data.line       = line;

    __disable_irq();

    __BKPT(0);

    for (;;)
    {
        __NOP();
    }
}
