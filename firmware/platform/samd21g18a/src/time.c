#include "platform/samd21g18a/time.h"
#include "sam.h" // IWYU pragma: keep
#include <stdint.h>

static uint32_t volatile time_msec = 0u;

void
platform_samd21g18a_time_init (void)
{
    (void)SysTick_Config(SystemCoreClock / 1000u);

    return;
}

uint32_t
platform_samd21g18a_time_msec (void)
{
    uint32_t current_time_ms;

    __disable_irq();
    current_time_ms = time_msec;
    __enable_irq();

    return current_time_ms;
}

uint32_t
platform_samd21g18a_time_usec (void)
{
    uint32_t msec_1;
    uint32_t msec_2;
    uint32_t systick_value;

    do
    {
        msec_1        = time_msec;
        systick_value = SysTick->VAL;
        msec_2        = time_msec;
    } while (msec_1 != msec_2);

    uint32_t systick_load   = SysTick->LOAD + 1u;
    uint32_t elapsed_cycles = systick_load - systick_value;
    uint32_t elapsed_usec   = (elapsed_cycles * 1000u) / systick_load;

    return (msec_1 * 1000u) + elapsed_usec;
}

void
platform_samd21g18a_time_sleep_msec (uint32_t sleep_msec)
{
    uint32_t start_time = platform_samd21g18a_time_msec();

    while ((platform_samd21g18a_time_msec() - start_time) < sleep_msec)
    {
        __WFI();
    }

    return;
}

void
platform_samd21g18a_time_sleep_usec (uint32_t sleep_usec)
{
    uint64_t start_time = platform_samd21g18a_time_usec();

    while ((platform_samd21g18a_time_usec() - start_time) < sleep_usec)
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
    time_msec++;

    return;
}
