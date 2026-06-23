#ifndef PLATFORM_SAMD21G18A_TIME_H
#define PLATFORM_SAMD21G18A_TIME_H

#include <stdint.h>

/** @brief Configure `SysTick` to interrupt every millisecond. */
void platform_samd21g18a_time_init(void);

/** @brief Return the number of milliseconds elapsed since initialization. */
uint32_t platform_samd21g18a_time_ms(void);

/** @brief Blocking delay for a number of milliseconds. */
void platform_samd21g18a_time_sleep_ms(uint32_t sleep_ms);

#endif
