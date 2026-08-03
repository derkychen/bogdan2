#include "platform/samd21g18a/assert.h"
#include "sam.h" // IWYU pragma: keep

assert_data_t volatile assert_data;

_Noreturn void
assert_fail (char const *expression, char const *file, int line)
{
    assert_data.expression = expression;
    assert_data.file       = file;
    assert_data.line       = line;

    __disable_irq();

    __BKPT(0);

    for (;;)
    {
        __NOP();
    }
}
