#ifndef PLATFORM_SAMD21G18A_USB_H
#define PLATFORM_SAMD21G18A_USB_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Configure USB and initialize TinyUSB device. */
void usb_init(void);

/**
 * @brief Wrapper around `tud_task`.
 *
 * NOTE: This function should be called periodically within the main loop.
 */
void usb_task(void);

/** @brief Wrapper around `tud_mounted`. */
bool usb_is_mounted(void);

#endif
