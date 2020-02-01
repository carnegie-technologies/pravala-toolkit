
cmake_minimum_required(VERSION 3.6)

set(NATIVE_ANDROID_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(NATIVE_BUILD_TYPE_DIR "debug")
else()
  set(NATIVE_BUILD_TYPE_DIR "release")
endif()

set(JAVA_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

# Returns a list of files to use as classpath arguments for javah.
# It stores the result in the first argument.
# All additional arguments are treated as either file paths,
# or as names of targets under java/ directory.
function(get_classpath_files files)
  set(ALL_FILES "")
  foreach(A ${ARGN})
    set(A_DIR "${JAVA_ROOT}/${A}/build/intermediates/compile_library_classes")
    if (EXISTS "${A}")
      list(APPEND ALL_FILES ${A})
    elseif(EXISTS "${A_DIR}/${NATIVE_BUILD_TYPE_DIR}/classes.jar")
      list(APPEND ALL_FILES "${A_DIR}/${NATIVE_BUILD_TYPE_DIR}/classes.jar")
    elseif(RELEASE_CUST AND EXISTS "${A_DIR}/release/classes.jar")
      list(APPEND ALL_FILES "${A_DIR}/release/classes.jar")
    else()
      message(FATAL_ERROR "Could not find classpath dependency for '${A}'")
    endif()
  endforeach()
  set(${files} ${ALL_FILES} PARENT_SCOPE)
endfunction()

# Macro to add commands to generate headers with javah
# Parameters:
#   header_file The name/path of the header file
#   symbol      The symbol to generate the header for (com.pravala.x.Y)
#   ...         Additional elements to use in the classpath list (passed to get_classpath_files).
#
# It will generate a command that outputs 'header_file' in the current binary directory,
# that depends on all classpath dependencies as well as all .class files in class_dir.
macro(native_autogen_header header_file symbol)
  set(TMP_NATIVE_DIR "${CMAKE_SOURCE_DIR}/build/intermediates/javac/${NATIVE_BUILD_TYPE_DIR}/classes")
  file(GLOB TmpNativeClassFiles ${TMP_NATIVE_DIR}/*.class)
  get_classpath_files(TMP_NATIVE_FILES ${ARGN})
  string(REPLACE ";" ":" TMP_NATIVE_CP "${TMP_NATIVE_FILES}")

  add_custom_command(
    OUTPUT ${header_file}
    COMMAND ${CMAKE_COMMAND} -E remove ${header_file}
    COMMAND javah
      -classpath "${TMP_NATIVE_DIR}:${TMP_NATIVE_CP}"
      -o ${header_file}
      ${symbol}
    DEPENDS ${TmpNativeClassFiles} ${TMP_NATIVE_FILES}
  )
endmacro()

native_autogen_header(SocketUtils.h com.pravala.socket.SocketUtils ${NATIVE_HEADER_DEPS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(SocketUtils SHARED jni/SocketUtilsImpl.c SocketUtils.h)
target_link_libraries(SocketUtils log)
