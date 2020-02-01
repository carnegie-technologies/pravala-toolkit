
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/os/${SYSTEM_TYPE}")
  return()
endif()

if (NOT TARGET LibWpaCtrl)
  return()
endif()

include_directories(${3RDPARTY_DIR})

file(GLOB LibWifiMgr_SRC *.cpp os/${SYSTEM_TYPE}/*.cpp)
add_library(LibWifiMgr ${LibWifiMgr_SRC})
target_link_libraries(LibWifiMgr LibLog LibWpaCtrl)
