/**
 * @file usb.h
 * @brief USB functionality.
 *
 * This module uses TinyUSB to provide USB utilities. The most important one is
 * `usb_task`, which must be called frequently in the application to avoid
 * starving the USB connection.
 *
 * @warning Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
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
