
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/os/${SYSTEM_TYPE}")
  return()
endif()

file(GLOB LibTun_SRC *.cpp os/${SYSTEM_TYPE}/*.cpp)
add_library(LibTun ${LibTun_SRC})
target_link_libraries(LibTun LibNet LibNetMgr)
