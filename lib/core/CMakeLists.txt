
set(MY_LIBS LibBasic LibProto LibError LibSys LibAutoError LibAutoNetwork)

# Those libraries don't explicitly expose their symbols, so we need to make them fully visible:
if(NOT MSVC)
  set_target_properties(${MY_LIBS} PROPERTIES COMPILE_FLAGS -fvisibility=default)
endif()

add_library_wrappers(PravCore ${MY_LIBS})
