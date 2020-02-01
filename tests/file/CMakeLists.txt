
file(GLOB FileTest_SRC *.cpp)
add_executable(FileTest ${FileTest_SRC})
target_link_libraries(FileTest LibBasic)

# Just build it, we don't run it...
add_dependencies(tests FileTest)
