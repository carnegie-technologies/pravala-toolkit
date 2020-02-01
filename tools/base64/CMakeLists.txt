
file(GLOB Base64_SRC *.cpp)
add_executable(Base64 ${Base64_SRC})
target_link_libraries(Base64 LibBase64)

add_dependencies(tools Base64)
