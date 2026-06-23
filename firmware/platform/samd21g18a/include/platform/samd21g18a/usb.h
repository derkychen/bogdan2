#ifndef PLATFORM_SAMD21G18A_USB_H
#define PLATFORM_SAMD21G18A_USB_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Configure USB and initialize TinyUSB device. */
bool platform_samd21g18a_usb_init(void);

/**
 * @brief Wrapper around `tud_task`.
 *
 * This function should be called periodically within the main loop.
 */
void platform_samd21g18a_usb_task(void);

/** @brief Wrapper around `tud_mounted`. */
bool platform_samd21g18a_usb_is_mounted(void);

#endif
