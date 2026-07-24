#include "tusb.h" // IWYU pragma: keep

#define USB_VID (0xCAFEu)
#define USB_PID (0x4001u)
#define USB_BCD (0x0200u)

#define USB_ITF_NUM_CDC      (0u)
#define USB_ITF_NUM_CDC_DATA (1u)
#define USB_ITF_NUM_TOTAL    (2u)

#define USB_EP_NUM_CDC_NOTIF (0x81u)
#define USB_EP_NUM_CDC_OUT   (0x02u)
#define USB_EP_NUM_CDC_IN    (0x82u)

#define USB_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

static tusb_desc_device_t const usb_device_descriptor
    = { .bLength         = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB          = USB_BCD,

        .bDeviceClass    = TUSB_CLASS_MISC,
        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol = MISC_PROTOCOL_IAD,

        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor  = USB_VID,
        .idProduct = USB_PID,
        .bcdDevice = 0x0100u,

        .iManufacturer = 0x01u,
        .iProduct      = 0x02u,
        .iSerialNumber = 0x03u,

        .bNumConfigurations = 0x01u };

static uint8_t const usb_configuration_descriptor[]
    = { TUD_CONFIG_DESCRIPTOR(1,
                              USB_ITF_NUM_TOTAL,
                              0,
                              USB_CONFIG_TOTAL_LEN,
                              TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                              100),

        TUD_CDC_DESCRIPTOR(USB_ITF_NUM_CDC,
                           4,
                           USB_EP_NUM_CDC_NOTIF,
                           8,
                           USB_EP_NUM_CDC_OUT,
                           USB_EP_NUM_CDC_IN,
                           64) };

static char const *usb_string_descriptors[] = { (const char[]) { 0x09, 0x04 },
                                                "MPSD",
                                                "Bogdan 2 Beam Profiler",
                                                "AR0006728",
                                                "CDC Interface" };

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

    static uint16_t descriptor_string[32];
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

        char const *str = usb_string_descriptors[index];

        chr_count = 0u;

        while ((str[chr_count] != '\0') && (chr_count < 31u))
        {
            descriptor_string[1u + chr_count] = (uint16_t)str[chr_count];
            chr_count++;
        }
    }

    descriptor_string[0]
        = (uint16_t)((TUSB_DESC_STRING << 8u) | (2u * chr_count + 2u));

    return descriptor_string;
}
