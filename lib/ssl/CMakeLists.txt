
if (NOT OPENSSL_FOUND)
  message(WARNING "Cannot build LibSsl - SSL library was not found")
  return()
endif()

# Currently there are build issues with LibSsl on Windows...
if(${SYSTEM_TYPE} STREQUAL "Windows")
  return()
endif()

# We use this Valgrind header to get rid of 'undefined data' warning caused by OpenSSL (see SslSocket.cpp)
find_path(VALGRIND_INCLUDE_DIR valgrind/memcheck.h)

if (VALGRIND_INCLUDE_DIR)
  add_definitions(-DUSE_VALGRIND_MEMCHECK=1)
  include_directories(${VALGRIND_INCLUDE_DIR})
endif()

include_directories(${OPENSSL_INCLUDE_DIR})
file(GLOB LibSsl_SRC *.cpp)
add_library(LibSsl ${LibSsl_SRC})
target_link_libraries(LibSsl LibSocket ${OPENSSL_LIBRARIES})
