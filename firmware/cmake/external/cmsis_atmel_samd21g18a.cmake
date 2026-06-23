add_library(cmsis_atmel_samd21g18a INTERFACE)

target_include_directories(cmsis_atmel_samd21g18a SYSTEM INTERFACE
  "${EXTERNAL_DIR}/cmsis-atmel/CMSIS-Atmel/CMSIS/Device/ATMEL"
  "${EXTERNAL_DIR}/cmsis-atmel/CMSIS-Atmel/CMSIS/Device/ATMEL/samd21/include"
)

target_compile_definitions(cmsis_atmel_samd21g18a INTERFACE
  __SAMD21G18A__
)

target_link_libraries(cmsis_atmel_samd21g18a INTERFACE
  external::cmsis_core
)

add_library(external::cmsis_atmel_samd21g18a ALIAS cmsis_atmel_samd21g18a)
