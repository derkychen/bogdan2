#ifndef PLATFORM_SAMD21G18A_UTILS_H
#define PLATFORM_SAMD21G18A_UTILS_H

#include <stdint.h>

/** @brief Poll the GCLK until it is synchronized. */
void platform_samd21g18a_utils_gclk_poll_sync(void);

/**
 * @brief Enable a peripheral with GCLK0.
 *
 * NOTE: This function assumes that GCLK0 has already been configured.
 */
void platform_samd21g18a_utils_gclk0_enable(uint16_t id);

#endif
