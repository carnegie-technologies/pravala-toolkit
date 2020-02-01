
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/os/${SYSTEM_TYPE}")
  return()
endif()

file(GLOB LibNetMgr_SRC *.cpp os/${SYSTEM_TYPE}/*.cpp)
add_library(LibNetMgr ${LibNetMgr_SRC})
target_link_libraries(LibNetMgr LibLog)

if ( ${SYSTEM_TYPE} STREQUAL "QNX" )
  target_link_libraries(LibNetMgr socket)
endif()
