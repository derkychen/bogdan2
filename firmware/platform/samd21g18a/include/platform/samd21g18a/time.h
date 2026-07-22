#ifndef PLATFORM_SAMD21G18A_TIME_H
#define PLATFORM_SAMD21G18A_TIME_H

#include <stdint.h>

/** @brief Configure `SysTick` to interrupt every millisecond. */
void platform_samd21g18a_time_init(void);

/** @brief Return the number of milliseconds elapsed since initialization. */
uint32_t platform_samd21g18a_time_msec(void);

/** @brief Return the number of microseconds elapsed since initialization. */
uint32_t platform_samd21g18a_time_usec(void);

/**
 * @brief Blocking delay for a number of milliseconds.
 *
 * NOTE: Interrupts are still handled in this function. However, it is
 *       recommended that this function is only called for delays well below 10
 *       milliseconds, as any more is very likely to starve other tasks.
 */
void platform_samd21g18a_time_sleep_msec(uint32_t sleep_msec);

/**
 * @brief Blocking delay for a number of microseconds.
 *
 * WARNING: Interrupts are not handled in this function. It is recommended that
 *          this function is only called for delays well below one millisecond.
 *          Any more is likely to inhibit other system functions.
 */
void platform_samd21g18a_time_sleep_usec(uint32_t sleep_usec);

#endif
