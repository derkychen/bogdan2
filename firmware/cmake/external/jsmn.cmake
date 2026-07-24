add_library(jsmn INTERFACE)

target_include_directories(jsmn SYSTEM INTERFACE
  "${EXTERNAL_DIR}/jsmn"
)

add_library(external::jsmn ALIAS jsmn)
