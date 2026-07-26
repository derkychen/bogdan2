add_library(warnings INTERFACE)

target_compile_options(warnings INTERFACE
  -Wall
  -Wconversion
  -Werror
  -Wextra
  -Wmaybe-uninitialized
  -Wshadow
  -Wswitch-enum
  -Wunused-but-set-variable
)

add_library(options::warnings ALIAS warnings)
