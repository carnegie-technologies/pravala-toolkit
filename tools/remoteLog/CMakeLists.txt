
file(GLOB RemoteLog_SRC *.cpp)
add_executable(RemoteLog ${RemoteLog_SRC})
target_link_libraries(RemoteLog LibApp LibCtrl)
install(TARGETS RemoteLog RUNTIME DESTINATION bin)
