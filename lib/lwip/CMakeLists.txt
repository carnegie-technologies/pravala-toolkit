
if(NOT TARGET lwip)
  return()
endif()

include_directories(${3RDPARTY_BIN_DIR}/lwip/lwip/src/include)
file(GLOB LibLwip_SRC *.cpp internal/*.cpp os/${SYSTEM_TYPE}/*.cpp)
add_library(LibLwip ${LibLwip_SRC})
target_link_libraries(LibLwip LibNet lwip)
