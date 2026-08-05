add_library(jsmn INTERFACE)

target_include_directories(jsmn SYSTEM INTERFACE
  "${jsmn_SOURCE_DIR}"
)

add_library(external::jsmn ALIAS jsmn)
