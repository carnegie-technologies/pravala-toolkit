
if (NOT TARGET LibDBus)
  return()
endif()

file(GLOB DBusTest_SRC *.cpp)
add_executable(DBusTest ${DBusTest_SRC})
target_link_libraries(DBusTest LibApp LibDBus)

# Just build it, we don't run it...
add_dependencies(tests DBusTest)
