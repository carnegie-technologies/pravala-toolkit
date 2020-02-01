
if (TARGET LibGps)
  file(GLOB GpsTest_SRC *.cpp)
  add_executable(GpsTest ${GpsTest_SRC})
  target_link_libraries(GpsTest LibGps)

  # Just build it, we don't run it...
  add_dependencies(tests GpsTest)
endif (TARGET LibGps)
