
if (NOT TARGET LibSsl)
  return()
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp DEPENDS AutoEcfgSslTest)
add_custom_target(AutoEcfgSslTest
  COMMAND EcfgGen
    -p "Ssl Test"
    -d "${PROJECT_SOURCE_DIR}"
    -i "${CMAKE_CURRENT_SOURCE_DIR}/local.ecfg"
    -o "${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp"
    ${ECFG_OPTIONS})

include_directories(${OPENSSL_INCLUDE_DIR})

file(GLOB SslTest_SRC *.cpp)
add_executable(SslTest ${SslTest_SRC} ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp)
target_link_libraries(SslTest LibApp LibSsl)

# Just build it, we don't run it...
add_dependencies(tests SslTest)
