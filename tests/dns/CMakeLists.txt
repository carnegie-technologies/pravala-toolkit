
file(GLOB DnsTest_SRC *.cpp)
add_executable(DnsTest ${DnsTest_SRC})
target_link_libraries(DnsTest LibApp LibDns)

# Just build it, we don't run it...
add_dependencies(tests DnsTest)
