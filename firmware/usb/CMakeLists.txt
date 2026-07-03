add_library(usb STATIC
  "${CMAKE_CURRENT_LIST_DIR}/src/descriptors.c"
)

target_include_directories(usb PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(usb
  PRIVATE
    mcu::samd21g18a
  PUBLIC
    external::tinyusb_device_samd
)
