add_library(cmsis_core INTERFACE)

target_include_directories(cmsis_core SYSTEM INTERFACE
  "${cmsis_SOURCE_DIR}/CMSIS/Core/Include"
)
