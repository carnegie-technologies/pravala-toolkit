
if (NOT TARGET LibCurl)
  return()
endif()

include_directories(${LIBCURL_INCLUDE_DIRS})
file(GLOB CurlTest_SRC *.cpp)
add_executable(CurlTest ${CurlTest_SRC})
target_link_libraries(CurlTest LibCurl LibApp)

# Just build it, we don't run it...
add_dependencies(tests CurlTest)
