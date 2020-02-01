
if (TARGET LibWifiMgr)
  file(GLOB WifiMgrTest_SRC *.cpp)
  add_executable(WifiMgrTest ${WifiMgrTest_SRC})
  target_link_libraries(WifiMgrTest LibApp LibWifiMgr)

  # Just build it, we don't run it...
  add_dependencies(tests WifiMgrTest)
endif (TARGET LibWifiMgr)
