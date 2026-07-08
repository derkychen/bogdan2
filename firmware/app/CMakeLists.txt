add_library(app OBJECT)

target_sources(app PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/src/main.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/serial.c"
)

target_include_directories(app PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(app
  PRIVATE
    mcu::samd21g18a
  PUBLIC
    board_indio
    external::jsmn
    external::tinyusb_device_samd
    platform_samd21g18a
)
