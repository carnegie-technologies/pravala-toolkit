
file(GLOB LibConfig_SRC *.cpp)
add_library(LibConfig ${LibConfig_SRC})
target_link_libraries(LibConfig LibError)
