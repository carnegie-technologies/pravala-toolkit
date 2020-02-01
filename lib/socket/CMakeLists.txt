
if (NOT TARGET LibLog)
  return()
endif()

include_directories(${3RDPARTY_DIR})

if (NOT UDP_SOCKET_IMPL)
  if(${SYSTEM_TYPE} STREQUAL "Android")

    # For sendmmsg/recvmmsg on Android we need LibSysCall that provides symbols that are missing in Bionic.
    # Unfortunately the i386 version of the LibSysCall doesn't build properly, and it's not trivial to fix.
    # We also need kernel 3.0 or higher. 32 bit ARM builds may be run on older devices.
    # So we only use mmsg on 64-bit Android architectures and iovec everywhere else.
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "64")
      set(UDP_SOCKET_IMPL "mmsg")
    else()
      set(UDP_SOCKET_IMPL "iovec")
    endif()

  elseif (${SYSTEM_TYPE} STREQUAL "Linux")
    set(UDP_SOCKET_IMPL "mmsg")
  elseif(${SYSTEM_TYPE} STREQUAL "BSD" OR ${SYSTEM_TYPE} STREQUAL "QNX" OR ${SYSTEM_TYPE} STREQUAL "Apple")
    set(UDP_SOCKET_IMPL "iovec")
  else()
    set(UDP_SOCKET_IMPL "basic")
  endif()
endif()

if (${UDP_SOCKET_IMPL} STREQUAL "iovec")
  add_definitions(-DUSE_UDP_IMPL_IOVEC=1)
elseif (${UDP_SOCKET_IMPL} STREQUAL "mmsg")
  add_definitions(-DUSE_UDP_IMPL_MMSG=1)
elseif (${UDP_SOCKET_IMPL} STREQUAL "basic")
  add_definitions(-DUSE_UDP_IMPL_BASIC=1)
else()
  message (FATAL_ERROR "Invalid UDP_SOCKET_IMPL implementation: ${UDP_SOCKET_IMPL}")
endif()

message (STATUS "Using UDP_SOCKET_IMPL implementation: ${UDP_SOCKET_IMPL}")

file(GLOB LibSocket_SRC *.cpp os/${SYSTEM_TYPE}/*.cpp)

if(ENABLE_VHOSTNET)
  file(GLOB LibSocket_VhostNet_SRC os/shared/vhostnet/*.cpp)
  set_source_files_properties(${LibSocket_VhostNet_SRC} PROPERTIES COMPILE_FLAGS -fpermissive)
endif()

add_library(LibSocket ${LibSocket_SRC} ${LibSocket_VhostNet_SRC})
target_link_libraries(LibSocket LibLog)

if(${UDP_SOCKET_IMPL} STREQUAL "mmsg" AND (NOT HAVE_SENDMMSG OR NOT HAVE_RECVMMSG))
  target_link_libraries(LibSocket LibSysCall)
endif()
