
file(GLOB AsyncDnsTest_SRC *.cpp)
add_executable(AsyncDnsTest ${AsyncDnsTest_SRC})
target_link_libraries(AsyncDnsTest LibAsyncDns LibApp)

# Just build it, we don't run it...
add_dependencies(tests AsyncDnsTest)
