add_library(tinyusb_device_samd STATIC)

target_sources(tinyusb_device_samd PRIVATE
  "${EXTERNAL_DIR}/tinyusb/src/tusb.c"
  "${EXTERNAL_DIR}/tinyusb/src/common/tusb_fifo.c"
  "${EXTERNAL_DIR}/tinyusb/src/device/usbd.c"
  "${EXTERNAL_DIR}/tinyusb/src/class/cdc/cdc_device.c"
  "${EXTERNAL_DIR}/tinyusb/src/portable/microchip/samd/dcd_samd.c"
)

target_include_directories(tinyusb_device_samd SYSTEM PUBLIC
  "${EXTERNAL_DIR}/tinyusb/src"
)

target_link_libraries(tinyusb_device_samd
  PRIVATE
    mcu::samd21g18a
    options::features
  PUBLIC
    usb
    external::cmsis_atmel_samd21g18a
)

add_library(external::tinyusb_device_samd ALIAS tinyusb_device_samd)
