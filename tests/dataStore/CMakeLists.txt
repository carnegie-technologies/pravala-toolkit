
file(GLOB DataStoreTest_SRC *.cpp)
add_executable(DataStoreTest ${DataStoreTest_SRC})
target_link_libraries(DataStoreTest LibSocket)

# Just build it, we don't run it...
add_dependencies(tests DataStoreTest)
