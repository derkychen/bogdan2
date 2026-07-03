/** @brief TinyUSB configuration macros. */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#define CFG_TUSB_MCU          (OPT_MCU_SAMD21)
#define CFG_TUSB_OS           (OPT_OS_NONE)
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE)
#define CFG_TUSB_DEBUG        (0u)

#define CFG_TUD_ENABLED        (1u)
#define CFG_TUD_ENDPOINT0_SIZE (64u)
#define CFG_TUD_CDC            (1u)
#define CFG_TUD_MSC            (0u)
#define CFG_TUD_HID            (0u)
#define CFG_TUD_MIDI           (0u)
#define CFG_TUD_VENDOR         (0u)
#define CFG_TUD_CDC_RX_BUFSIZE (512u)
#define CFG_TUD_CDC_TX_BUFSIZE (512u)

#endif
