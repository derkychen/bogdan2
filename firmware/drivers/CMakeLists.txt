add_library(drivers OBJECT)

target_sources(drivers PRIVATE
  #"${CMAKE_CURRENT_LIST_DIR}/src/mcp342x.c"
  #"${CMAKE_CURRENT_LIST_DIR}/src/mcp4726.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/pca9555.c"
)

target_include_directories(drivers PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(drivers
  PRIVATE
    mcu::samd21g18a
  PUBLIC
    platform_samd21g18a
)
