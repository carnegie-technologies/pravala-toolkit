
file(GLOB UnitTest_SRC *.cpp ${PROJECT_SOURCE_DIR}/tests/unit/UnitTest.cpp)
add_executable(UnitTestLibSys ${UnitTest_SRC})
target_link_libraries(UnitTestLibSys gtest LibSys)

add_custom_target(runUnitTestLibSys ${CMAKE_CURRENT_BINARY_DIR}/UnitTestLibSys DEPENDS UnitTestLibSys)
add_dependencies(tests runUnitTestLibSys)
