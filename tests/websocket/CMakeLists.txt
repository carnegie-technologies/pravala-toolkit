
if (TARGET LibWebSocket)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp DEPENDS AutoEcfgWebSocketTest)
  add_custom_target(AutoEcfgWebSocketTest
    COMMAND EcfgGen
      -p "WebSocket Test"
      -d "${PROJECT_SOURCE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp"
      ${ECFG_OPTIONS})

  file(GLOB WebSocketTest_SRC *.cpp)
  add_executable(WebSocketTest ${WebSocketTest_SRC} ${CMAKE_CURRENT_BINARY_DIR}/auto_ecfg.cpp)
  target_link_libraries(WebSocketTest LibApp LibWebSocket)

  # Just build it, we don't run it...
  add_dependencies(tests WebSocketTest)
endif (TARGET LibWebSocket)
