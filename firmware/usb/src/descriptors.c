/**
 * @file descriptors.c
 * @brief Implements TinyUSB descriptor callbacks.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 */
#include "tusb.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VID (0xCAFEu)
#define PID (0x4001u)
#define BCD (0x0200u)

#define STRING_INDEX_MANUFACTURER  (0x01u)
#define STRING_INDEX_PRODUCT       (0x02u)
#define STRING_INDEX_SERIAL_NUMBER (0x03u)
#define STRING_INDEX_CDC           (0x04u)

#define ITF_NUMBER_CDC      (0u)
#define ITF_NUMBER_CDC_DATA (1u)
#define ITF_NUMBER_TOTAL    (2u)

#define EP_NUMBER_CDC_NOTIFICATIONS (0x81u)
#define EP_NUMBER_CDC_OUT           (0x02u)
#define EP_NUMBER_CDC_IN            (0x82u)

#define CFG_TOTAL_LENGTH (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

#define SERIAL_NUMBER_LENGTH              (32u)
#define SERIAL_NUMBER_WORDS               (4u)
#define SERIAL_NUMBER_HEX_DIGITS_PER_WORD (8u)

#define STRING_DESCRIPTOR_MAX_CHARACTERS (32u)

_Static_assert(
    SERIAL_NUMBER_LENGTH
        == SERIAL_NUMBER_WORDS * SERIAL_NUMBER_HEX_DIGITS_PER_WORD,
    "Serial number length does not match processor serial number size.");

_Static_assert(SERIAL_NUMBER_LENGTH <= STRING_DESCRIPTOR_MAX_CHARACTERS,
               "USB string descriptor length is too small for serial number.");

static tusb_desc_device_t const usb_device_descriptor
    = { .bLength         = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB          = BCD,

        .bDeviceClass    = TUSB_CLASS_MISC,
        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol = MISC_PROTOCOL_IAD,

        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor  = VID,
        .idProduct = PID,
        .bcdDevice = 0x0100u,

        .iManufacturer = STRING_INDEX_MANUFACTURER,
        .iProduct      = STRING_INDEX_PRODUCT,
        .iSerialNumber = STRING_INDEX_SERIAL_NUMBER,

        .bNumConfigurations = 0x01u };

static uint8_t const usb_configuration_descriptor[]
    = { TUD_CONFIG_DESCRIPTOR(1,
                              ITF_NUMBER_TOTAL,
                              0,
                              CFG_TOTAL_LENGTH,
                              TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                              100),

        TUD_CDC_DESCRIPTOR(ITF_NUMBER_CDC,
                           STRING_INDEX_CDC,
                           EP_NUMBER_CDC_NOTIFICATIONS,
                           8,
                           EP_NUMBER_CDC_OUT,
                           EP_NUMBER_CDC_IN,
                           64) };

static char const *const usb_string_descriptors[]
    = { (const char[]) { 0x09, 0x04 },
        "MPSD",
        "Bogdan 2 Beam Profiler",
        NULL,
        "CDC Interface" };

static char serial_number[SERIAL_NUMBER_LENGTH + 1u];
static bool serial_number_initialized = false;

/**
 * @brief Get the unique 128-bit processor serial number as a hexadecimal
 *        string.
 */
static char const *
get_serial_number (void)
{
    if (!serial_number_initialized)
    {
        static uintptr_t const uid_addresses[SERIAL_NUMBER_WORDS] = {
            0x0080A00Cu,
            0x0080A040u,
            0x0080A044u,
            0x0080A048u,
        };

        static char const hex_digits[] = "0123456789ABCDEF";

        size_t output_index = 0u;

        for (size_t word_index = 0u; word_index < SERIAL_NUMBER_WORDS;
             word_index++)
        {
            uint32_t const word
                = *(volatile uint32_t const *)uid_addresses[word_index];

            for (uint32_t nibble_index = 0u;
                 nibble_index < SERIAL_NUMBER_HEX_DIGITS_PER_WORD;
                 nibble_index++)
            {
                uint32_t const shift
                    = (SERIAL_NUMBER_HEX_DIGITS_PER_WORD - 1u - nibble_index)
                      * 4u;

                serial_number[output_index++]
                    = hex_digits[(word >> shift) & 0x0Fu];
            }
        }

        serial_number[output_index] = '\0';
        serial_number_initialized   = true;
    }

    return serial_number;
}

uint8_t const *
tud_descriptor_device_cb (void)
{
    return (uint8_t const *)&usb_device_descriptor;
}

uint8_t const *
tud_descriptor_configuration_cb (uint8_t index)
{
    (void)index;

    return usb_configuration_descriptor;
}

uint16_t const *
tud_descriptor_string_cb (uint8_t index, uint16_t langid)
{
    (void)langid;

    static uint16_t descriptor_string[STRING_DESCRIPTOR_MAX_CHARACTERS + 1u];
    uint8_t         chr_count;

    if (index == 0u)
    {
        descriptor_string[1] = 0x0409u;
        chr_count            = 1u;
    }
    else
    {
        if (index >= (sizeof(usb_string_descriptors)
                      / sizeof(usb_string_descriptors[0])))
        {
            return NULL;
        }

        char const *str;

        if (index == STRING_INDEX_SERIAL_NUMBER)
        {
            str = get_serial_number();
        }
        else
        {
            str = usb_string_descriptors[index];
        }

        chr_count = 0u;

        while ((str[chr_count] != '\0')
               && (chr_count < STRING_DESCRIPTOR_MAX_CHARACTERS))
        {
            descriptor_string[1u + chr_count] = (uint16_t)str[chr_count];
            chr_count++;
        }
    }

    descriptor_string[0]
        = (uint16_t)((TUSB_DESC_STRING << 8u) | (2u * chr_count + 2u));

    return descriptor_string;
}
