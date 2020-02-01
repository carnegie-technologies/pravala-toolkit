
file(GLOB LibError_SRC *.cpp)
add_library(LibError ${LibError_SRC})
target_link_libraries(LibError LibAutoError)
