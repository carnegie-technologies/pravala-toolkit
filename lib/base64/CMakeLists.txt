
include_directories(${3RDPARTY_DIR}/libb64/libb64/include)
file(GLOB LibBase64_SRC *.cpp)
add_library(LibBase64 ${LibBase64_SRC})
target_link_libraries(LibBase64 LibBasic LibB64)
