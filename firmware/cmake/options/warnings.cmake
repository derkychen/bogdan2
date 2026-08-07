add_library(project_warnings INTERFACE)

target_compile_options(project_warnings INTERFACE
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
