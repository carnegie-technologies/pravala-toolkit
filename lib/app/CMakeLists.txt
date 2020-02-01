
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/os/${SYSTEM_TYPE}")
  return()
endif()

include_directories(${3RDPARTY_DIR})

file(GLOB LibApp_SRC *.cpp os/${SYSTEM_TYPE}/*.cpp)
add_library(LibApp ${LibApp_SRC})

target_link_libraries(LibApp LibSocket)

if(${SYSTEM_TYPE} STREQUAL "Windows")
  target_link_libraries(LibApp LibWindows)
endif()
