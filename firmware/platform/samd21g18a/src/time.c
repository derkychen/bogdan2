/**
 * @file time.c
 * @brief Implementation of timing functionality.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
#include "platform/samd21g18a/time.h"
#include "sam.h" // IWYU pragma: keep
#include <stdint.h>

static uint32_t volatile ms = 0u;

void
time_init (void)
{
    (void)SysTick_Config(SystemCoreClock / 1000u);

    return;
}

uint32_t
time_get_ms (void)
{
    return ms;
}

uint32_t
time_get_us (void)
{
    uint32_t ms_value;
    uint32_t systick_value;
    uint32_t systick_pending;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    ms_value        = ms;
    systick_value   = SysTick->VAL;
    systick_pending = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;

    // Account for if `SysTick` has wrapped but `SysTick_Handler()` has not
    // incremented `ms`.
    if (systick_pending != 0u)
    {
        ms_value++;
        systick_value = SysTick->VAL;
    }

    __set_PRIMASK(primask);

    uint32_t systick_load = SysTick->LOAD + 1u;

    uint32_t elapsed_cycles;

    if (systick_value != 0u)
    {
        elapsed_cycles = (SysTick->LOAD - systick_value) + 1u;
    }
    else
    {
        elapsed_cycles = 0u;
    }

    uint32_t elapsed_us = (elapsed_cycles * 1000u) / systick_load;

    return (ms_value * 1000u) + elapsed_us;
}

void
time_sleep_ms (uint32_t sleep_ms)
{
    uint32_t start_time = time_get_ms();

    while ((time_get_ms() - start_time) < sleep_ms)
    {
        __WFI();
    }

    return;
}

void
time_sleep_us (uint32_t sleep_us)
{
    uint32_t start_time = time_get_us();

    while ((time_get_us() - start_time) < sleep_us)
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
    ms++;

    return;
}
