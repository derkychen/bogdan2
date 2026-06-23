add_library(samd21g18a INTERFACE)

target_compile_options(samd21g18a INTERFACE
  -mcpu=cortex-m0plus
  -mthumb
  -ffunction-sections
  -fdata-sections
  -fno-common
  -fno-unwind-tables
  -fno-asynchronous-unwind-tables
  -Wall
  -Wextra
)

target_link_options(samd21g18a INTERFACE
  -mcpu=cortex-m0plus
  -mthumb
  -nostartfiles
  --specs=nano.specs
  --specs=nosys.specs
  "LINKER:--gc-sections"
)

add_library(mcu::samd21g18a ALIAS samd21g18a)
