
file(GLOB UnitTest_SRC *.cpp ${PROJECT_SOURCE_DIR}/tests/unit/UnitTest.cpp)
add_executable(UnitTestLibBase64 ${UnitTest_SRC})
target_link_libraries(UnitTestLibBase64 gtest LibBase64)

add_custom_target(runUnitTestLibBase64 ${CMAKE_CURRENT_BINARY_DIR}/UnitTestLibBase64 DEPENDS UnitTestLibBase64)
add_dependencies(tests runUnitTestLibBase64)
