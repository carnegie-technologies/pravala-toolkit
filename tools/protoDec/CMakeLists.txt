
file(GLOB ProtoDec_SRC *.cpp)
add_executable(ProtoDec ${ProtoDec_SRC})
target_link_libraries(ProtoDec LibApp)
add_dependencies(tools ProtoDec)
