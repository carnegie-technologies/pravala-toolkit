file(GLOB CalendarTimeTest_SRC *.cpp)
add_executable(CalendarTimeTest ${CalendarTimeTest_SRC})
target_link_libraries(CalendarTimeTest LibSys)

# Just build it, we don't run it...
add_dependencies(tests CalendarTimeTest)
