add_library(mcu_samd21g18a INTERFACE)

target_compile_options(mcu_samd21g18a INTERFACE
  -mcpu=cortex-m0plus
  -mthumb
)

target_link_options(mcu_samd21g18a INTERFACE
  -mcpu=cortex-m0plus
  -mthumb
)
