/**
 * @file assert.c
 * @brief Implements the function that handles a failed assertion.
 *
 * @warning Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
#include "platform/samd21g18a/assert.h"
#include "sam.h" // IWYU pragma: keep

assert_data_t volatile assert_data;

void
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
