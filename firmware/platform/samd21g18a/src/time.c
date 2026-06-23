#include "platform/samd21g18a/time.h"
#include "samd21g18a.h"
#include <stdint.h>

static volatile uint32_t time_ms = 0u;

void
platform_samd21g18a_time_init (void)
{
    (void)SysTick_Config(SystemCoreClock / 1000u);

    return;
}

uint32_t
platform_samd21g18a_time_ms (void)
{
    uint32_t current_time_ms;

    __disable_irq();
    current_time_ms = time_ms;
    __enable_irq();

    return current_time_ms;
}

void
platform_samd21g18a_time_sleep_ms (uint32_t delay_ms)
{
    uint32_t start_time;

    start_time = platform_samd21g18a_time_ms();

    while ((platform_samd21g18a_time_ms() - start_time) < delay_ms)
    {
        __WFI();
    }

    return;
}

/** @brief Overrides the `SysTick_Handler` function in the vector table. */
void
SysTick_Handler (void)
{
    time_ms++;

    return;
}
