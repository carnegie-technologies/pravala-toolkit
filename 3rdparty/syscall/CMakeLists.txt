
# Functions from LibSysCall C library
if(NOT CMAKE_SYSTEM_PROCESSOR)
  message(FATAL_ERROR "CMAKE_SYSTEM_PROCESSOR must be set to build LibSysCall")
endif()

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
  set(LibSysCall_ARCH "i386")
elseif ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm" )
  set(LibSysCall_ARCH "arm")
elseif ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^mips" )
  set(LibSysCall_ARCH "mips")
else()
  set(LibSysCall_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

message (STATUS "Using SysCall_ARCH=${LibSysCall_ARCH}")

add_definitions(-DLIB_SYSCALL_ARCH_${LibSysCall_ARCH}=1)

enable_language(ASM)
file(GLOB LibSysCall_SRC ${LibSysCall_ARCH}/*.s)

if(NOT HAVE_SENDMMSG)
  set(SRCS ${SRCS} sendmmsg.c)
  message (STATUS "LibSysCall enabling sendmmsg")
endif()

if(NOT HAVE_RECVMMSG)
  set(SRCS ${SRCS} recvmmsg.c)
  message (STATUS "LibSysCall enabling recvmmsg")
endif()

add_library(LibSysCall syscall.c ${SRCS} ${LibSysCall_SRC})
