/**
 * @file utils.h
 * @brief Miscellaneous utilities functions.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
#ifndef PLATFORM_SAMD21G18A_UTILS_H
#define PLATFORM_SAMD21G18A_UTILS_H

#include <stdint.h>

/**
 * @brief Enable a peripheral with GCLK0.
 *
 * NOTE: This function assumes that GCLK0 has already been configured.
 */
void utils_gclk0_enable(uint16_t id);

/** @brief Enable a processor writing to a peripheral with APBC mask. */
void utils_apbc_enable(uint32_t mask);

#endif
