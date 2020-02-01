
if (NOT TARGET LibEvent)
  return()
endif()

file(GLOB LibLog_SRC *.cpp os/${SYSTEM_TYPE}/*.cpp os/${SYSTEM_TYPE}/*.m)

add_library(LibLog ${LibLog_SRC} ${LibLog_VhostNet_SRC})
target_link_libraries(LibLog LibAutoLog LibEvent)

if(${SYSTEM_TYPE} STREQUAL "Android")
  target_link_libraries(LibLog log)
elseif(${SYSTEM_TYPE} STREQUAL "Apple")
  target_link_libraries(LibLog "-framework Foundation")
endif()
