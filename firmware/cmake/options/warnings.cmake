add_library(warnings INTERFACE)

target_compile_options(warnings INTERFACE
  $<$<C_COMPILER_ID:GNU>:-Wmaybe-uninitialized>
  -Wall
  -Wconversion
  -Wdouble-promotion
  -Werror
  -Wextra
  -Wshadow
  -Wswitch-enum
  -Wunused-but-set-variable
)

add_library(options::warnings ALIAS warnings)
