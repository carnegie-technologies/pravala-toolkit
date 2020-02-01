
if (ENABLE_LIBEVENT AND NOT LIBEVENT_FOUND)
  message(WARNING "Cannot build LibEvent - ENABLE_LIBEVENT is set, but libevent was not found")
  return()
endif()

# If is not used LIBEVENT_INCLUDE_DIRS should be empty anyway:
include_directories(${LIBEVENT_INCLUDE_DIRS} ${3RDPARTY_DIR})

if (ENABLE_LIBEVENT)
  add_definitions("-DUSE_LIBEVENT=1")
endif()

file(GLOB LibEvent_SRC *.cpp os/${SYSTEM_TYPE}/*.cpp)
add_library(LibEvent ${LibEvent_SRC})

# If is not used LIBEVENT_LIBRARIES should be empty anyway:
target_link_libraries(LibEvent LibConfig LibSys LibSimpleLog ${LIBEVENT_LIBRARIES})
