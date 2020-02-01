
file(GLOB LibAsyncDns_SRC *.cpp )
add_library(LibAsyncDns ${LibAsyncDns_SRC})

target_link_libraries(LibAsyncDns LibLog LibDns)
