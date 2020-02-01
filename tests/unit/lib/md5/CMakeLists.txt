
file(GLOB UnitTest_SRC *.cpp ${PROJECT_SOURCE_DIR}/tests/unit/UnitTest.cpp)
add_executable(UnitTestLibMd5 ${UnitTest_SRC})
target_link_libraries(UnitTestLibMd5 gtest LibMd5)

add_custom_target(runUnitTestLibMd5 ${CMAKE_CURRENT_BINARY_DIR}/UnitTestLibMd5 DEPENDS UnitTestLibMd5)
add_dependencies(tests runUnitTestLibMd5)
