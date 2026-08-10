/**
 * @file time.h
 * @brief Timing functionality.
 *
 * This module provides bare-bones timing functionality. Waits are usually
 * blocking, so they should be used carefully.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
#ifndef PLATFORM_SAMD21G18A_TIME_H
#define PLATFORM_SAMD21G18A_TIME_H

#include <stdint.h>

/** @brief Configure `SysTick` to interrupt every millisecond. */
void time_init(void);

/** @brief Return the number of milliseconds elapsed since initialization. */
uint32_t time_get_ms(void);

/** @brief Return the number of microseconds elapsed since initialization. */
uint32_t time_get_us(void);

/**
 * @brief Blocking delay for a number of milliseconds.
 *
 * NOTE: Interrupts are still handled in this function. However, it is
 *       recommended that this function is only called for delays well below 10
 *       milliseconds, as any more is very likely to starve other tasks.
 */
void time_sleep_ms(uint32_t sleep_ms);

/**
 * @brief Blocking delay for a number of microseconds.
 *
 * WARNING: Interrupts are not handled in this function. It is recommended that
 *          this function is only called for delays well below one millisecond.
 *          Any more is likely to inhibit other system functions.
 */
void time_sleep_us(uint32_t sleep_us);

#endif
