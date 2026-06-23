add_library(cmsis_core INTERFACE)

target_include_directories(cmsis_core SYSTEM INTERFACE
  "${EXTERNAL_DIR}/cmsis/CMSIS/Core/Include"
)

add_library(external::cmsis_core ALIAS cmsis_core)
