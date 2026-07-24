add_library(features INTERFACE)

target_compile_options(features INTERFACE
  -ffunction-sections
  -fdata-sections
  -fno-common
  -fno-unwind-tables
  -fno-asynchronous-unwind-tables
)

add_library(options::features ALIAS features)
