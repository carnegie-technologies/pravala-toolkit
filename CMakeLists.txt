#
# Project's main CMakeLists.txt file
#
# Possible options:
#
# -D CMAKE_INSTALL_PREFIX=/usr/local
#
# -D CMAKE_BUILD_TYPE=Debug
# -D CMAKE_BUILD_TYPE=Release
# -D CMAKE_BUILD_TYPE=RelWithDebInfo
#
# -D STATIC_BINARIES=true
#
# -D ENABLE_PROFILING=false
# -D ENABLE_PROFILING=true
#
# -D ENABLE_PERFTOOLS=false
# -D ENABLE_PERFTOOLS=true
#
# -D ENABLE_LIBEVENT=true
# -D ENABLE_POLL=true
#
# -D DISABLE_SIGNALFD=true
#
# -D REVISION_SUFFIX=build_revision_suffix
#
# Enable the frame-pointer to allow for oprofile to generate call-graphs
# -D ENABLE_FRAME_POINTER=true
#
# Tune for a certain CPU type, restricting/optimizing instructions:
# See gcc manpage for valid options.
#
# -D CPU_MARCH=generic|native|corei7|corei7-avx|core-avx-i|atom|geode|...
#
# Set UDP socket implementation to use on supported platforms:
#
# -D UDP_SOCKET_IMPL=basic|iovec|mmsg
#
# basic = sendto/recvfrom   (all)
# iovec = sendmsg/recvfrom  (most POSIX)
# mmsg  = sendmmsg/recvmmsg (Linux only)
#
# Defaults are:
#
# cmake -D INSTALL_DIR=. -D CMAKE_BUILD_TYPE=Debug -D ENABLE_PROFILING=false ..
#
# No Debug:
#
# cmake -D INSTALL_DIR=. -D CMAKE_BUILD_TYPE=Release -D ENABLE_PROFILING=false ..
#
# If you need to edit this file, or any other CMakeLists.txt file, check the documentation here:
# http://www.cmake.org/cmake/help/cmake-2-8-docs.html
# (it's WAY better than Googling)
#

project(PToolkit)

cmake_minimum_required(VERSION 3.5)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

if(INSTALL_DIR)
  set(CMAKE_INSTALL_PREFIX "${INSTALL_DIR}")
else()
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")
endif()

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  set(EXEC_SUFFIX ".exe")
endif()

# Core module. It sets various variables to be used by this project,
# as well as any other projects using the core project.
# Variables defined in CMakeLists.txt files of each project are
# invisible to higher projects. Variables defined
# in the modules - are (in each project that includes the module)

include(Core)
include(Docker)

set(3RDPARTY_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(3RDPARTY_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/3rdparty)

add_subdirectory(3rdparty EXCLUDE_FROM_ALL)

if(NOT AUTO_HDR_DIR)
  set(AUTO_HDR_DIR "${CMAKE_BINARY_DIR}/auto_headers")
endif()

# We also need to propagate it up (if this is not the top-level project):

if(NOT "${PROJECT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  set(AUTO_HDR_DIR "${AUTO_HDR_DIR}" PARENT_SCOPE)
endif()

message(STATUS "Using AUTO_HDR_DIR: '${AUTO_HDR_DIR}'")

include_directories(${PROJECT_SOURCE_DIR}/lib ${AUTO_HDR_DIR})

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")

  if( NOT ${SYSTEM_TYPE} STREQUAL "QNX" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
  endif()
endif()

add_subdirectory(tools)

# Let's also create convenience macros for auto-generating protocol code

# Macro to autogenerate targets for automatic code generation.
# Parameters:
#   output_dir The output directory where the code should be generated.
#   base_name  The base name of a single proto file. For 'foo_bar.proto' it is 'foo_bar'.
#   proto_dir  The directory where 'base_name.proto' file is.
#   ...        Additional dependencies to add.
#
# It will generate a command, that outputs '_base_name.cpp' in the current binary directory,
# as well as 'output_dir/auto/base_name/.generated'.
#
# That command will generate the code using 'proto_dir/name.proto' and put it under output_dir.
# The implementation file will be put in '_base_name.cpp' in the current binary directory.
#
# Also, if ${PROTO_AUTOGEN_JSON} is set, generated code will support JSON serialization.
macro(proto_autogen output_dir base_name proto_dir)
  unset(TMP_PROTO_DEPS)
  unset(TMP_PROTO_ARGS)
  if(PROTO_AUTOGEN_JSON)
    set(TMP_PROTO_ARGS "--enable-json")
  endif()
  foreach(TMP_PROTO_DEP ${ARGN})
    set(TMP_PROTO_DEPS ${TMP_PROTO_DEPS} ${TMP_PROTO_DEP})
  endforeach()
  add_custom_command(
    OUTPUT ${output_dir}/auto/${base_name}/.generated _${base_name}.cpp
    COMMAND ${CMAKE_COMMAND} -E remove _${base_name}.cpp
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${output_dir}/auto/${base_name}
    COMMAND ProtoGen cpp
      -o ${output_dir}
      -f _${base_name}.cpp
      --time-flag-file=${output_dir}/auto/${base_name}/.generated
      --dir-prefix=auto
      --use-proto-file-as-dir-prefix
      --id-scope=branch
      --namespace-prefix=Pravala
      --skip-leading-dirs="Pravala/Protocol, Pravala"
      ${TMP_PROTO_ARGS}
      ${proto_dir}/${base_name}.proto
    DEPENDS ${proto_dir}/${base_name}.proto ${TMP_PROTO_DEPS})
endmacro()

# Macro to autogenerate targets for automatic code generation.
# It works just like proto_autogen, but is easier to use in the common case.
# It doesn't take the output_dir and uses ${AUTO_HDR_DIR}.
# Also, all dependencies (extra arguments) are assumed to be base_names of other
# .proto files in the same proto_dir that the generated command should depend on.
macro(easy_proto_autogen base_name proto_dir)
  unset(TMP_PROTO_DEPS)
  foreach(TMP_PROTO_DEP ${ARGN})
    set(TMP_PROTO_DEPS ${TMP_PROTO_DEPS} ${proto_dir}/${TMP_PROTO_DEP}.proto)
  endforeach()
  proto_autogen(${AUTO_HDR_DIR} ${base_name} ${proto_dir} ${TMP_PROTO_DEPS})
endmacro()

# Returns a list of dependencies of targets passed as parameters.
# This operates recursively, and will return the entire dependency tree.
# It only returns dependencies that are cmake targets - external libraries or other tokens,
# like linker files, are not returned.
# Targets passed as arguments are included as well.
# The list will not have any duplicates.
function(get_link_dependencies output_list)
  set(ALL_DEPS "")
  foreach(T ${ARGN})
    if (TARGET ${T})
      list(APPEND ALL_DEPS ${T})
      get_target_property(LIBS ${T} LINK_LIBRARIES)
      foreach(L ${LIBS})
        if (TARGET ${L})
          get_link_dependencies(TMP ${L})
          list(APPEND ALL_DEPS ${TMP})
        endif()
      endforeach()
    endif()
  endforeach()
  list(REMOVE_DUPLICATES ALL_DEPS)
  set(${output_list} ${ALL_DEPS} PARENT_SCOPE)
endfunction()

# Creates a "library wrapper" using one or more static libraries, without additional source files.
#
# This wrapper consists of a shared and a static library. Those library targets use
# "SharedLib" and "Lib" prefixes, so passing "FooBar" as a name will result in "SharedLibFooBar"
# and "LibFooBar" library targets.
#
# The static library is created without adding additional source files and will simply add
# all arguments passed as its dependencies.
#
# The shared library will include all static libraries passed as arguments to this function.
# It will also include all static dependencies of those libraries
# (but only the symbols actually used by at least one of the static libraries passed as parameters).
# Also, the OUTPUT_NAME property of this target will be set to only include the name passed,
# which will generate a "libFooBar.so" file (instead of "libSharedLibFooBar.so").
#
# Parameters:
#  name The name of the wrapper, will be used with "Lib" (for static library target)
#       and "SharedLib" (for shared library target) prefixes.
#  ...  All static libraries to include (using -Wl,--whole-archive in the shared library).
function(add_library_wrappers name)

  # To create target we need at least one source file. So let's create one. It will be empty, but that's ok.
  # Also, let's use C extension so it's compatible with both C and CPP builds.
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/_${name}.c)

  set(STATIC_TARGET Lib${name})
  set(SHARED_TARGET SharedLib${name})

  if (NOT TARGET ${STATIC_TARGET})
    add_library(${STATIC_TARGET} ${CMAKE_CURRENT_BINARY_DIR}/_${name}.c)
    target_link_libraries(${STATIC_TARGET} ${ARGN})
  endif()

  if (NOT TARGET ${SHARED_TARGET})
    add_library(${SHARED_TARGET} SHARED ${CMAKE_CURRENT_BINARY_DIR}/_${name}.c)
    target_link_libraries(${SHARED_TARGET} "-Wl,--whole-archive" ${ARGN} "-Wl,--no-whole-archive")
    set_target_properties(${SHARED_TARGET} PROPERTIES OUTPUT_NAME ${name})
  endif()
endfunction()

# Real code
add_subdirectory(lib)

add_subdirectory(tests EXCLUDE_FROM_ALL)
