add_library(cmsis_core INTERFACE)

target_include_directories(cmsis_core SYSTEM INTERFACE
  "${cmsis_SOURCE_DIR}/CMSIS/Core/Include"
)

add_library(external::cmsis_core ALIAS cmsis_core)
