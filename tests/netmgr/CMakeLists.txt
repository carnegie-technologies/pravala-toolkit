
if (TARGET LibNetMgr)
  file(GLOB NetMgrTest_SRC *.cpp)
  add_executable(NetMgrTest ${NetMgrTest_SRC})
  target_link_libraries(NetMgrTest LibApp LibNetMgr)

  # Just build it, we don't run it...
  add_dependencies(tests NetMgrTest)
endif (TARGET LibNetMgr)
