
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp DEPENDS AutoEcfgSignalTest)
add_custom_target(AutoEcfgSignalTest
  COMMAND EcfgGen
    -p "Signal Test"
    -d "${PROJECT_SOURCE_DIR}"
    -i "${CMAKE_CURRENT_SOURCE_DIR}/local.ecfg"
    -o "${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp"
    ${ECFG_OPTIONS})

file(GLOB SignalTest_SRC *.cpp)
add_executable(SignalTest ${SignalTest_SRC} ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp)
target_link_libraries(SignalTest LibEvent)

# Just build it, we don't run it...
add_dependencies(tests SignalTest)
