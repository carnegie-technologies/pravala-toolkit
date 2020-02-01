
if(CMAKE_CROSSCOMPILING)
  # We want to build EcfgGen only when not cross-compiling
  return()
endif()

file(GLOB EcfgGen_SRC *.cpp)
add_executable(EcfgGen ${EcfgGen_SRC})
target_link_libraries(EcfgGen LibApp LibJson)
add_dependencies(tools EcfgGen)
