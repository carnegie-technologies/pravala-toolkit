
if (NOT TARGET LibTun)
  return()
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp DEPENDS AutoEcfgTunTest)
add_custom_target(AutoEcfgTunTest
  COMMAND EcfgGen
    -p "Tun Test"
    -d "${PROJECT_SOURCE_DIR}"
    -i "${CMAKE_CURRENT_SOURCE_DIR}/local.ecfg"
    -o "${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp"
    ${ECFG_OPTIONS})

file(GLOB TunTest_SRC *.cpp)
add_executable(TunTest ${TunTest_SRC} ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp)
target_link_libraries(TunTest LibApp LibTun LibNetMgr)

# Just build it, we don't run it...
add_dependencies(tests TunTest)
