add_library(platform_samd21g18a OBJECT)

target_sources(platform_samd21g18a PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/src/assert.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/digital.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/eic.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/evsys.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/i2c.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/pin.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/pulser.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/system.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/time.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/usb.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/utils.c"
)

target_include_directories(platform_samd21g18a PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(platform_samd21g18a
  PRIVATE
    mcu_samd21g18a
    project_build_options
    project_warnings
    tinyusb_device_samd
  PUBLIC
    cmsis_atmel_samd21g18a
)
