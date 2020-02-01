
file(GLOB LibPrometheus_SRC *.cpp internal/*.cpp)

add_library(LibPrometheus ${LibPrometheus_SRC})

target_link_libraries(LibPrometheus LibHttp)
set_target_properties(LibPrometheus PROPERTIES OUTPUT_NAME "Prometheus")
