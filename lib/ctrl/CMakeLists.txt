
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/os/${SYSTEM_TYPE}")
  return()
endif()

file(GLOB LibCtrl_SRC *.cpp cmds/*.cpp os/${SYSTEM_TYPE}/*.cpp)
add_library(LibCtrl ${LibCtrl_SRC})
target_link_libraries(LibCtrl LibAutoCtrl LibApp)
