add_library(project_build_options INTERFACE)

target_compile_options(project_build_options INTERFACE
  $<$<CONFIG:Debug>:-Og>
  $<$<CONFIG:Debug>:-g3>
  -fdata-sections
  -ffunction-sections
  -fno-asynchronous-unwind-tables
  -fno-common
  -fno-unwind-tables
)
