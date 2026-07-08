add_library(jsmn INTERFACE)

target_include_directories(jsmn INTERFACE
  "${EXTERNAL_DIR}/jsmn"
)

add_library(external::jsmn ALIAS jsmn)
