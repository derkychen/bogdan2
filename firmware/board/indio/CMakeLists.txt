add_library(board_indio OBJECT)

target_sources(board_indio PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/src/analog_output.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/io_cfg.c"
)

target_include_directories(board_indio PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(board_indio
  PRIVATE
    mcu::samd21g18a
    options::features
    options::warnings
  PUBLIC
    drivers
    platform_samd21g18a
)
