
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp DEPENDS AutoEcfgTimerTest)
add_custom_target(AutoEcfgTimerTest
  COMMAND EcfgGen
    -p "Timer Test"
    -d "${PROJECT_SOURCE_DIR}"
    -i "${CMAKE_CURRENT_SOURCE_DIR}/local.ecfg"
    -o "${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp"
    ${ECFG_OPTIONS})

file(GLOB TimerTest_SRC *.cpp)
add_executable(TimerTest ${TimerTest_SRC} ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp)
target_link_libraries(TimerTest LibApp)

# Just build it, we don't run it...
add_dependencies(tests TimerTest)
