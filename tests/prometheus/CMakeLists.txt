
add_executable(PrometheusTest main.cpp)
target_link_libraries(PrometheusTest LibApp LibPrometheus)

# Just build it, we don't run it...
add_dependencies(tests PrometheusTest)
