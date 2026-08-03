/**
 * @file time.c
 * @brief Implementation of timing functionality.
 */
#include "platform/samd21g18a/time.h"
#include "sam.h" // IWYU pragma: keep
#include <stdint.h>

static uint32_t volatile msecs = 0u;

void
time_init (void)
{
    (void)SysTick_Config(SystemCoreClock / 1000u);

    return;
}

uint32_t
time_msec (void)
{
    uint32_t current_time_ms;

    __disable_irq();
    current_time_ms = msecs;
    __enable_irq();

    return current_time_ms;
}

uint32_t
time_usec (void)
{
    uint32_t msecs_1;
    uint32_t msecs_2;
    uint32_t systick_value;

    do
    {
        msecs_1       = msecs;
        systick_value = SysTick->VAL;
        msecs_2       = msecs;
    } while (msecs_1 != msecs_2);

    uint32_t systick_load   = SysTick->LOAD + 1u;
    uint32_t elapsed_cycles = systick_load - systick_value;
    uint32_t elapsed_usec   = (elapsed_cycles * 1000u) / systick_load;

    return (msecs_1 * 1000u) + elapsed_usec;
}

void
time_sleep_msec (uint32_t sleep_msec)
{
    uint32_t start_time = time_msec();

    while ((time_msec() - start_time) < sleep_msec)
    {
        __WFI();
    }

    return;
}

void
time_sleep_usec (uint32_t sleep_usec)
{
    uint64_t start_time = time_usec();

    while ((time_usec() - start_time) < sleep_usec)
    {
        // NOTE: `__WFI()` is not used here due to the possibility of wake-up
        //       latency being longer than the actual delay.
        __NOP();
    }

    return;
}

/** @brief Overrides the `SysTick_Handler` function in the vector table. */
void
SysTick_Handler (void)
{
    msecs++;

    return;
}
