
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/os/${SYSTEM_TYPE}")
  return()
endif()

file(GLOB LibSys_SRC *.cpp os/${SYSTEM_TYPE}/*.cpp)

add_library(LibSys ${LibSys_SRC})
target_link_libraries(LibSys LibError)

if (ZLIB_FOUND)
  include_directories(${ZLIB_INCLUDE_DIRS})
  target_link_libraries(LibSys ${ZLIB_LIBRARIES})
endif()

if(${SYSTEM_TYPE} STREQUAL "Windows")
  target_link_libraries(LibSys wsock32)
elseif(${SYSTEM_TYPE} STREQUAL "QNX" )
  target_link_libraries(LibSys socket)
elseif(${SYSTEM_TYPE} STREQUAL "Apple" )
  include_directories(${PROJECT_SOURCE_DIR}/../3rdparty/sem_timedwait)
  target_link_libraries(LibSys LibSemTimedWait)
endif()
