add_library(app OBJECT)

target_sources(app PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/src/axis.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/controller.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/main.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/parameters.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/path.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/profiler.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/relay.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/serial.c"
)

target_include_directories(app PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(app
  PRIVATE
    mcu_samd21g18a
    project_build_options
    project_warnings
  PUBLIC
    board_indio
    jsmn
    platform_samd21g18a
    tinyusb_device_samd
)
