
if (NOT LIBCURL_FOUND)
  message(WARNING "Cannot build LibCurl/LibCurlCore - CURL library was not found")
  return()
elseif (STATIC_BINARIES)
  message (WARNING "LibCurl/LibCurlCore cannot be built in static mode")
  return()
endif()

include_directories(${LIBCURL_INCLUDE_DIRS})
add_definitions(${LIBCURL_DEFINES})

file(GLOB LibCurlCore_SRC core/*.cpp)
add_library(LibCurlCore ${LibCurlCore_SRC})

target_link_libraries(LibCurlCore LibSys LibError ${LIBCURL_LDFLAGS})

if(LIBMBEDTLS_LDFLAGS)
  target_link_libraries(LibCurlCore ${LIBMBEDTLS_LDFLAGS})
else()
  target_link_libraries(LibCurlCore ${OPENSSL_LIBRARIES})
endif()

if (${SYSTEM_TYPE} STREQUAL "QNX")
  target_link_libraries(LibCurlCore socket z)
endif()

file(GLOB LibCurl_SRC *.cpp)
add_library(LibCurl ${LibCurl_SRC})

target_link_libraries(LibCurl LibLog LibCurlCore)
