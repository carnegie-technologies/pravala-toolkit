
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp DEPENDS AutoEcfgSocks5Test)
add_custom_target(AutoEcfgSocks5Test
  COMMAND EcfgGen
    -p "SOCKS5 Test"
    -d "${PROJECT_SOURCE_DIR}"
    -i "${CMAKE_CURRENT_SOURCE_DIR}/local.ecfg"
    -o "${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp"
    ${ECFG_OPTIONS})

include_directories(${OPENSSL_INCLUDE_DIR})

file(GLOB Socks5Test_SRC *.cpp)
add_executable(Socks5Test ${Socks5Test_SRC} ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp)
target_link_libraries(Socks5Test LibApp LibSocks5)

# Just build it, we don't run it...
add_dependencies(tests Socks5Test)
