include_guard(GLOBAL)

include(FetchContent)

# CMSIS v6.3.0
FetchContent_Declare(
  cmsis
  GIT_REPOSITORY https://github.com/ARM-software/CMSIS_6.git
  GIT_TAG        45dab712ad84f8cbbf2b7bfc089c19088507df6f
  SOURCE_SUBDIR  _project_does_not_use_upstream_cmake
)

# CMSIS-Atmel v1.1.0
FetchContent_Declare(
  cmsis_atmel
  GIT_REPOSITORY https://github.com/arduino/ArduinoModule-CMSIS-Atmel.git
  GIT_TAG        df76b3a6c82eea57bcd962fda42bc4ffb3a3a1fa
  SOURCE_SUBDIR  _project_does_not_use_upstream_cmake
)

# jsmn
FetchContent_Declare(
  jsmn
  GIT_REPOSITORY https://github.com/zserge/jsmn.git
  GIT_TAG        25647e692c7906b96ffd2b05ca54c097948e879c
  SOURCE_SUBDIR  _project_does_not_use_upstream_cmake
)

# TinyUSB v0.21.0
FetchContent_Declare(
  tinyusb
  GIT_REPOSITORY https://github.com/hathach/tinyusb.git
  GIT_TAG        dae3f9a366bfcddbf9dcf1b48d7500286a849539
  SOURCE_SUBDIR  _project_does_not_use_upstream_cmake
)

FetchContent_MakeAvailable(
  tinyusb
  cmsis
  cmsis_atmel
  jsmn
)
