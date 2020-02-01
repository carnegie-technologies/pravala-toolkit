
if (TARGET LibNetMgr)
  file(GLOB NetMgrCtrlTest_SRC *.cpp)
  add_executable(NetMgrCtrlTest ${NetMgrCtrlTest_SRC})
  target_link_libraries(NetMgrCtrlTest LibApp LibNetMgr)

  # Just build it, we don't run it...
  add_dependencies(tests NetMgrCtrlTest)
endif (TARGET LibNetMgr)
