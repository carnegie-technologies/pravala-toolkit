
if (TARGET LibWifiMgr)
  file(GLOB WifiMgrCtrlTest_SRC *.cpp)
  add_executable(WifiMgrCtrlTest ${WifiMgrCtrlTest_SRC})
  target_link_libraries(WifiMgrCtrlTest LibApp LibWifiMgr)

  # Just build it, we don't run it...
  add_dependencies(tests WifiMgrCtrlTest)
endif (TARGET LibWifiMgr)
