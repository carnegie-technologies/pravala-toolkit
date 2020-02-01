
if (NOT TARGET LibCurlCore)
  return()
endif()

include_directories(${LIBCURL_INCLUDE_DIRS})
file(GLOB EasyCurlTest_SRC *.cpp)
add_executable(EasyCurlTest ${EasyCurlTest_SRC})
target_link_libraries(EasyCurlTest LibCurlCore)

# Just build it, we don't run it...
add_dependencies(tests EasyCurlTest)
