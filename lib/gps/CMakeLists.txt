
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/os/${SYSTEM_TYPE}")
  return()
endif()

if (NOT TARGET LibJson)
  return()
endif()

file(GLOB LibGps_SRC *.cpp os/${SYSTEM_TYPE}/*.cpp)
add_library(LibGps ${LibGps_SRC})
target_link_libraries(LibGps LibLog LibJson)
