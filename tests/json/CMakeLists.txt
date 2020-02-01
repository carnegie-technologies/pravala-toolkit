
if (TARGET LibJson)
  include_directories(${JANSSON_INCLUDE_DIRS})
  file(GLOB JsonTest_SRC *.cpp)
  add_executable(JsonTest ${JsonTest_SRC})
  target_link_libraries(JsonTest LibJson LibError)

  # Just build it, we don't run it...
  add_dependencies(tests JsonTest)
endif (TARGET LibJson)
