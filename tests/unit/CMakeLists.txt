
find_package(GTestSrc)

if (NOT GTEST_SRC_FOUND)
  message(WARNING "GTest sources were not found, so Unit Tests will NOT be built. "
                  "Try 'sudo apt-get install libgtest-dev' to install on Ubuntu")
  return()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-variadic-macros -Wno-pedantic -Wno-sign-compare")

add_subdirectory("${GTEST_SRC_DIR}" "${CMAKE_BINARY_DIR}/.gtest")

if (NOT TARGET gtest)
  message(WARNING "GTest sources were found (in ${GTEST_SRC_DIR}), but 'gtest' target does not exists. "
                  "Unit Tests will NOT be built")
  return()
endif()

message(STATUS "GTest found in ${GTEST_SRC_DIR} - Unit Tests WILL be built")

# We want to include '.' explicitly here. Thanks to this, all unit tests added by this file using add_subdirectory
# will include THIS directory as well. Otherwise, all of them would have to use include_directories(../..)
include_directories(. ${GTEST_INCLUDE_DIR})

add_subdirectory(lib)
add_subdirectory(proto)
