add_library(warnings INTERFACE)

target_compile_options(warnings INTERFACE
  -Wall
  -Wextra
  -Wmaybe-uninitialized
  -Wunused-but-set-variable
  -Wswitch-enum
  -Wshadow
  -Wconversion
)

add_library(options::warnings ALIAS warnings)
