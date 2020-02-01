
file(GLOB LibMd5_SRC *.cpp)
add_library(LibMd5 ${LibMd5_SRC})
target_link_libraries(LibMd5 LibBasic)
