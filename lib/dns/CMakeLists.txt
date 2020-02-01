
file(GLOB LibDns_SRC *.c )

include_directories(${3RDPARTY_DIR})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

add_library(LibDns ${LibDns_SRC})
target_link_libraries(LibDns LibSimpleLog)

if(${SYSTEM_TYPE} STREQUAL "Linux")
  target_link_libraries(LibDns pthread)
elseif(${SYSTEM_TYPE} STREQUAL "Android")
  target_link_libraries(LibDns dl)
endif()

add_library_wrappers(Dns LibDns)

# We include shared libraries that we want to depend on.
# This way symbols already provided by them won't be included in this one.
target_link_libraries(SharedLibDns SharedLibSimpleLog)
