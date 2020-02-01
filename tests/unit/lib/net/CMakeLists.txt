
file(GLOB UnitTest_SRC *.cpp ${PROJECT_SOURCE_DIR}/tests/unit/UnitTest.cpp)
add_executable(UnitTestLibNet ${UnitTest_SRC})
target_link_libraries(UnitTestLibNet gtest LibNet)

add_custom_target(runUnitTestLibNet ${CMAKE_CURRENT_BINARY_DIR}/UnitTestLibNet DEPENDS UnitTestLibNet)
add_dependencies(tests runUnitTestLibNet)
