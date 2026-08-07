add_library(tinyusb_device_samd STATIC)

target_sources(tinyusb_device_samd PRIVATE
  "${tinyusb_SOURCE_DIR}/src/tusb.c"
  "${tinyusb_SOURCE_DIR}/src/common/tusb_fifo.c"
  "${tinyusb_SOURCE_DIR}/src/device/usbd.c"
  "${tinyusb_SOURCE_DIR}/src/class/cdc/cdc_device.c"
  "${tinyusb_SOURCE_DIR}/src/portable/microchip/samd/dcd_samd.c"
)

target_include_directories(tinyusb_device_samd SYSTEM PUBLIC
  "${tinyusb_SOURCE_DIR}/src"
)

target_link_libraries(tinyusb_device_samd
  PRIVATE
    mcu_samd21g18a
    project_build_options
  PUBLIC
    usb
    cmsis_atmel_samd21g18a
)
