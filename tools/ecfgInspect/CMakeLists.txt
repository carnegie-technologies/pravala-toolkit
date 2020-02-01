
file(GLOB EcfgInspect_SRC *.cpp)
add_executable(EcfgInspect ${EcfgInspect_SRC})
target_link_libraries(EcfgInspect LibBasic)
add_dependencies(tools EcfgInspect)
