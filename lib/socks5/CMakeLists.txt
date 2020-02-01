
file(GLOB LibSocks5_SRC *.cpp internal/*.cpp)

add_library(LibSocks5 ${LibSocks5_SRC})
target_link_libraries(LibSocks5 LibSocket)

# This library doesn't explicitly expose its symbols, so we need to make them fully visible:
if(NOT MSVC)
  set_target_properties(LibSocks5 PROPERTIES COMPILE_FLAGS -fvisibility=default)
endif()

add_library_wrappers(Socks5 LibSocks5)

# We include shared libraries that we want to depend on.
# This way symbols already provided by them won't be included in this one.
target_link_libraries(SharedLibSocks5 SharedLibPravCore)
