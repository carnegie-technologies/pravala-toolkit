
file(GLOB LibNet_SRC *.cpp)
add_library(LibNet ${LibNet_SRC})
target_link_libraries(LibNet LibSocket)
