
include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE ProtoDesc_SRC ${PROJECT_SOURCE_DIR}/proto/tests/*.proto)

add_custom_command(
  OUTPUT auto/auto.cpp
  COMMAND ${CMAKE_COMMAND} -E remove_directory auto
  COMMAND ProtoGen cpp
    -o auto
    -f auto/auto.cpp
    --id-scope=branch
    --enable-json
    --namespace-prefix=Pravala.Protocol
    --skip-leading-dirs="Pravala/Protocol, Pravala"
    ${PROJECT_SOURCE_DIR}/proto/tests/test.proto
  DEPENDS ${ProtoDesc_SRC}
)

file(GLOB UnitTest_SRC *.cpp)
add_executable(UnitTestProto ${UnitTest_SRC} auto/auto.cpp
                             ${PROJECT_SOURCE_DIR}/tests/unit/UnitTest.cpp)
target_link_libraries(UnitTestProto gtest LibError LibJson)

add_custom_target(runUnitTestProto ${CMAKE_CURRENT_BINARY_DIR}/UnitTestProto DEPENDS UnitTestProto)
add_dependencies(tests runUnitTestProto)
