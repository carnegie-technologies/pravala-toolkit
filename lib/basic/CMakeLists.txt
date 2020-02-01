
file(GLOB LibBasic_SRC *.cpp internal/*.cpp os/${SYSTEM_TYPE}/*.cpp)
add_library(LibBasic ${LibBasic_SRC})

if ( ${SYSTEM_TYPE} STREQUAL "Windows")
  target_link_libraries(LibBasic ws2_32)
elseif ( ${SYSTEM_TYPE} STREQUAL "Linux" )
  target_link_libraries(LibBasic rt pthread)
endif()
