
if(NOT ${SYSTEM_TYPE} STREQUAL "Windows")
  return()
endif()

file(GLOB LibWindows_SRC *.cpp)
add_library(LibWindows ${LibWindows_SRC})
target_link_libraries(LibWindows LibError)
