
file(GLOB LibProto_SRC *.cpp)
add_library(LibProto ${LibProto_SRC})
target_link_libraries(LibProto LibBasic)
