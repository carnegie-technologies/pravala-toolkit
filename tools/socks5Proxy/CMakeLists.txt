
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp DEPENDS AutoEcfgSocks5Proxy)
add_custom_target(AutoEcfgSocks5Proxy
  COMMAND EcfgGen
    -p "SOCKS5 Proxy"
    -d "${PROJECT_SOURCE_DIR}"
    -i "${CMAKE_CURRENT_SOURCE_DIR}/local.ecfg"
    -o "${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp"
    ${ECFG_OPTIONS})

include_directories(${OPENSSL_INCLUDE_DIR})

file(GLOB Socks5Proxy_SRC *.cpp)
add_executable(Socks5Proxy ${Socks5Proxy_SRC} ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp)
target_link_libraries(Socks5Proxy LibSocks5 LibApp)
