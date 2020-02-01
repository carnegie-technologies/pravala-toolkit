
file(GLOB LibServerApp_SRC *.cpp)
add_library(LibServerApp ${LibServerApp_SRC})

target_link_libraries(LibServerApp LibApp LibPrometheus)
