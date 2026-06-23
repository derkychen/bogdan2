add_library(app OBJECT)

target_sources(app PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/src/main.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/usb_cdc.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c"
)

target_include_directories(app PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(app
  PRIVATE
    mcu::samd21g18a
  PUBLIC
    platform_samd21g18a
    external::tinyusb_device_samd
)
