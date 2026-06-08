cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(bogdan2)

pico_sdk_init()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(bogdan2
  axis.c
  config.c
  instruction.c
  main.c
  path.c
  pulse_counter.c
  serial.c
)

target_link_libraries(bogdan2
  pico_stdlib
  hardware_pwm
)

target_include_directories(bogdan2 PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/external/jsmn
)

pico_enable_stdio_usb(bogdan2 1)
pico_enable_stdio_uart(bogdan2 0)

# Output UF2
pico_add_extra_outputs(bogdan2)
