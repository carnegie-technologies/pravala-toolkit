
if (NOT DBUS_FOUND)
  return()
endif()

include_directories(${DBUS_INCLUDE_DIRS})
file(GLOB LibDBus_SRC *.cpp internal/*.cpp)
add_library(LibDBus ${LibDBus_SRC})
target_link_libraries(LibDBus LibLog ${DBUS_LDFLAGS})
