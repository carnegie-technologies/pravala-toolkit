
include_directories(${3RDPARTY_DIR}/http-parser)
file(GLOB LibHttp_SRC *.cpp)
add_library(LibHttp ${LibHttp_SRC})
target_link_libraries(LibHttp LibSocket LibHttpParser)
