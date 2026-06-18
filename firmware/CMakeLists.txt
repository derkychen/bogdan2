cmake_minimum_required(VERSION 3.25)

project(bogdan2_firmware C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(TARGET_NAME firmware.elf)

if(DEFINED ENV{CMSIS_PACK_ROOT})
  set(CMSIS_PACK_ROOT "$ENV{CMSIS_PACK_ROOT}")
elseif(WIN32)
  set(CMSIS_PACK_ROOT "$ENV{LOCALAPPDATA}/Arm/Packs")
else()
  set(CMSIS_PACK_ROOT "$ENV{HOME}/.cache/arm/packs")
endif()

set(SAMD21_DFP_VERSION "3.8.270")

set(SAMD21_DFP_ROOT
  "${CMSIS_PACK_ROOT}/Microchip/SAMD21_DFP/${SAMD21_DFP_VERSION}"
)

set(SAMD21A_DFP_INCLUDE_DIR
  "${SAMD21_DFP_ROOT}/samd21a/include"
)

set(CMSIS_INCLUDE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/external/cmsis/CMSIS/Core/Include"
)

set(JSMN_INCLUDE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/external/jsmn"
)

set(TINYUSB_INCLUDE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/external/tinyusb/src"
)

set(APP_INCLUDE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/app/include"
)

set(PLATFORM_SAMD21G18A_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/platform/samd21g18a"
)

set(PLATFORM_SAMD21G18A_LINKER_DIR
  "${PLATFORM_SAMD21G18A_DIR}/linker"
)

set(LINKER_SCRIPT_PATH
  "${PLATFORM_SAMD21G18A_LINKER_DIR}/samd21g18a_bootloader_0x4000.ld"
)

message(STATUS "SAMD21 DFP include: ${SAMD21_DFP_INCLUDE_DIR}")
message(STATUS "CMSIS Core include: ${CMSIS_INCLUDE_DIR}")
message(STATUS "Linker script: ${LINKER_SCRIPT_PATH}")

add_executable(${TARGET_NAME}
  platform/samd21g18a/startup_samd21g18a.c
  platform/samd21g18a/system_samd21g18a.c
  app/src/main.c
)

target_include_directories(${TARGET_NAME} PRIVATE
  "${SAMD21A_DFP_INCLUDE_DIR}"
  "${CMSIS_INCLUDE_DIR}"
  "${JSMN_INCLUDE_DIR}"
  "${TINYUSB_INCLUDE_DIR}"
  "${APP_INCLUDE_DIR}"
)

target_compile_definitions(${TARGET_NAME} PRIVATE __SAMD21G18A__)

target_compile_options(${TARGET_NAME} PRIVATE
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

target_link_options(${TARGET_NAME} PRIVATE
  -mcpu=cortex-m0plus
  -mthumb
  -nostartfiles
  --specs=nano.specs
  --specs=nosys.specs
  "LINKER:-T,${LINKER_SCRIPT_PATH}"
  "LINKER:--gc-sections"
  "LINKER:-Map,${CMAKE_CURRENT_BINARY_DIR}/firmware.map"
  "LINKER:--print-memory-usage"
)

add_custom_command(
  TARGET ${TARGET_NAME}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> firmware.bin
  COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}> firmware.hex
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
  BYPRODUCTS
      firmware.bin
      firmware.hex
      firmware.map
  COMMENT "Generating firmware.bin and firmware.hex"
)
