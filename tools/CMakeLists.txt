
# To build all tools using a single target:
add_custom_target(tools)

add_subdirectory(base64)
add_subdirectory(protoGen)
add_subdirectory(protoDec)
add_subdirectory(ecfgGen)
add_subdirectory(ecfgInspect)
add_subdirectory(socks5Proxy)
add_subdirectory(remoteLog)
