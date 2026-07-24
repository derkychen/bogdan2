add_library(platform_samd21g18a OBJECT)

target_sources(platform_samd21g18a PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/src/assert.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/digital.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/eic.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/i2c.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/system.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/time.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/usb.c"
)

target_include_directories(platform_samd21g18a PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(platform_samd21g18a
  PRIVATE
    mcu::samd21g18a
    options::features
    options::warnings
    external::tinyusb_device_samd
  PUBLIC
    external::cmsis_atmel_samd21g18a
)
