add_library(warnings INTERFACE)

target_compile_options(warnings INTERFACE
  -Wall
  -Wconversion
  -Wdouble-promotion
  -Werror
  -Wextra
  $<$<C_COMPILER_ID:GNU>:-Wmaybe-uninitialized>
  -Wshadow
  -Wswitch-enum
  -Wunused-but-set-variable
)

add_library(options::warnings ALIAS warnings)
