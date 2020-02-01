
file(GLOB LibSimpleLog_SRC *.c os/${SYSTEM_TYPE}/*.m)
add_library(LibSimpleLog ${LibSimpleLog_SRC})

if (${SYSTEM_TYPE} STREQUAL "Android")
  target_link_libraries(LibSimpleLog log)
endif()

add_library_wrappers(SimpleLog LibSimpleLog)
