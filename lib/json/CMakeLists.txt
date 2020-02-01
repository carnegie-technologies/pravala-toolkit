
if (NOT TARGET jansson)
  return()
endif()

include_directories(${3RDPARTY_BIN_DIR}/jansson/jansson/include)
file(GLOB LibJson_SRC *.cpp)
add_library(LibJson ${LibJson_SRC})
target_link_libraries(LibJson LibBasic jansson)
