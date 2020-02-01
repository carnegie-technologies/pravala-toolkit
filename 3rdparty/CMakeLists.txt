
if(${SYSTEM_TYPE} STREQUAL "Linux" OR ${SYSTEM_TYPE} STREQUAL "Android")
  add_subdirectory(syscall)
endif()

if(NOT ${SYSTEM_TYPE} STREQUAL "Windows")
  add_subdirectory(lwip)
  add_subdirectory(wpa)
endif()

add_subdirectory(sqlite)
add_subdirectory(jansson)
add_subdirectory(libb64)

add_library(LibHttpParser http-parser/http_parser.c)
add_library(LibSemTimedWait sem_timedwait/sem_timedwait.c)
