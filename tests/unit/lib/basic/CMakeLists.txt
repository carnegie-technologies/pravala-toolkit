
file(GLOB UnitTest_SRC *.cpp ${PROJECT_SOURCE_DIR}/tests/unit/UnitTest.cpp)
add_executable(UnitTestLibBasic ${UnitTest_SRC})
target_link_libraries(UnitTestLibBasic gtest LibBasic)

add_custom_target(runUnitTestLibBasic ${CMAKE_CURRENT_BINARY_DIR}/UnitTestLibBasic DEPENDS UnitTestLibBasic)
add_dependencies(tests runUnitTestLibBasic)
