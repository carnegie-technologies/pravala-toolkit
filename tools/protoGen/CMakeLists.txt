
if(CMAKE_CROSSCOMPILING)
  # We want to build ProtoGen only when not cross-compiling
  return()
endif()

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
endif()

file(GLOB ProtoGen_SRC *.cpp */*.cpp)
add_executable(ProtoGen ${ProtoGen_SRC})
target_link_libraries(ProtoGen LibBasic LibMd5)
add_dependencies(tools ProtoGen)
