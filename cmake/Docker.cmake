
if (DOCKER_CMAKE_MODULE_PROCESSED)
  return()
endif()

set (DOCKER_CMAKE_MODULE_PROCESSED 1)

cmake_minimum_required(VERSION 3.5)

add_custom_target(docker_build)
add_custom_target(docker_push)

execute_process(COMMAND docker version --format {{.Client.Version}} OUTPUT_VARIABLE DOCKER_VERSION)
string(REGEX REPLACE "\n$" "" DOCKER_VERSION "${DOCKER_VERSION}")

message(STATUS "Docker version: ${DOCKER_VERSION}")

if ("${DOCKER_VERSION}" VERSION_LESS "17.06")
  # We need docker >= 17.06. That version added support for '--iidfile' argument that we use.
  message(WARNING "Docker not found or too old (version: '${DOCKER_VERSION}'); Docker targets will NOT be generated")
  macro(add_docker_targets)
  endmacro()
  macro(add_docker_targets_and_deps)
  endmacro()
  macro(configure_docker_file)
  endmacro()
  return()
endif()

# Macro to autogenerate targets for building and pushing docker images.
# It will create docker_build_NAME and docker_push_NAME targets.
# It does not and add them as dependencies of docker_build and docker_push,
# to have them added as well, use add_docker_targets_and_deps instead.
# Arguments:
# name - the name of the main target. This binary will be included in the generated image.
# ...  - additional items to include. They will all be placed in the '/' folder of the generated image.
#        If that item is the name of a file inside 'CURRENT_SRC_DIR/deploy/' folder, it will simply be copied.
#        Otherwise it will be treated as the name of a cmake target, and that target will be generated and copied.
#        If there is 'deploy/Dockerfile', it will be copied to the build directory.
#        If there is no such file, the caller must make sure that it is generated in some other way.
#        There is also a helper macro (configure_docker_file),
#        that can be used to generate Dockerfile based on Dockerfile.in.
#        The exact same thing applies to metadata.json, except there is no helper macro for this one.
macro(add_docker_targets name)
  set(TAR_CONTENT ${name} ${ARGN})
  set(TAR_DEPS "")
  set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deploy)

  foreach(T ${TAR_CONTENT})
    set(SRC ${SRC_DIR}/${T})
    list(APPEND TAR_DEPS ${CMAKE_CURRENT_BINARY_DIR}/${T})

    if(TARGET ${T})
      # It's a target
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${T}
        DEPENDS ${T}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${T}> ./${T})
    elseif(EXISTS ${SRC_DIR}/${T})
      # It's a file inside deploy/
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${T}
        DEPENDS ${SRC_DIR}/${T}
        COMMAND ${CMAKE_COMMAND} -E copy ${SRC_DIR}/${T} ./${T})

    # Otherwise a custom rule needs to be provided to create it for us.
    endif()
  endforeach()

  if(EXISTS ${SRC_DIR}/Dockerfile)
    add_custom_command(
      OUTPUT Dockerfile
      DEPENDS ${SRC_DIR}/Dockerfile
      COMMAND ${CMAKE_COMMAND} -E copy ${SRC_DIR}/Dockerfile ./)
  endif()

  if(EXISTS ${SRC_DIR}/metadata.json)
    add_custom_command(
      OUTPUT metadata.json
      DEPENDS ${SRC_DIR}/metadata.json
      COMMAND ${CMAKE_COMMAND} -E copy ${SRC_DIR}/metadata.json ./)
  endif()

  add_custom_command(
    OUTPUT ${name}.tar.gz
    DEPENDS ${TAR_DEPS}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${name}.tar.gz
    COMMAND ${CMAKE_COMMAND} -E tar czf ${name}.tar.gz ${TAR_CONTENT})

  add_custom_command(
    OUTPUT ${name}_docker_iid
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Dockerfile ${name}.tar.gz
    COMMAND docker build --iidfile ${name}_docker_iid .
    COMMENT "Building ${name} docker image")

  add_custom_target(docker_build_${name}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name}_docker_iid
    COMMAND sh -c "echo Verifying ${name} docker image [$(cat ${name}_docker_iid)]:"
    COMMAND sh -c "docker run $(cat ${name}_docker_iid) /${name} --version"
    VERBATIM)

  add_custom_target(docker_push_${name}
    DEPENDS docker_build_${name} ${CMAKE_CURRENT_BINARY_DIR}/metadata.json
    COMMAND
        ${PROJECT_SOURCE_DIR}/build-scripts/bin/run.bash
        node
        ${PROJECT_SOURCE_DIR}/build-scripts/deploy/ctEcrPushHelper.js
        ${CMAKE_CURRENT_BINARY_DIR}/)
endmacro()

# A wrapper around add_docker_targets that also adds generated targets as dependencies
# to global docker targets (docker_build and docker_push).
# Arguments: see add_docker_targets
macro(add_docker_targets_and_deps name)
  add_docker_targets(${name} ${ARGN})
  add_dependencies(docker_build docker_build_${name})
  add_dependencies(docker_push docker_push_${name})
endmacro()

# A helper macro that generates Dockerfile based on Dockerfile.in.
# At the moment it checks the OpenSSL version used by the host OS,
# and configures Dockerfile to use specific base docker image that includes correct libssl version.
# It will read deploy/Dockerfile.in file, and replace @DOCKER_IMAGE_BASE@ with
# either ubuntu:xenial or ubuntu:bionic.
# It will also replace @DOCKER_IMAGE_PACKAGES@ with either libssl1.0.0 or libssl1.1.
# Arguments:
# name - the name of the main target. This is only used for logging.
macro(configure_docker_file name)
  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/deploy/Dockerfile.in)
    message(FATAL_ERROR "${CMAKE_CURRENT_SOURCE_DIR}/deploy/Dockerfile.in does not exist")
  endif()

  set(DOCKER_IMAGE_BASE ubuntu:xenial)

  # We cannot use VERSION_LESS_EQUAL or VERSION_GREATER_EQUAL, because they were added in newish CMake versions...
  # So this is somewhat weird:

  if ("${OPENSSL_VERSION}" VERSION_LESS "1.0")
    message(WARNING "Unknown/unsupported OpenSSL version (${OPENSSL_VERSION})")
  elseif ("${OPENSSL_VERSION}" VERSION_LESS "1.1")
    # Yes, Ubuntu uses 'libssl1.0.0' package name, even if the actual version is, for instance, 1.0.2.
    set(DOCKER_IMAGE_PACKAGES libssl1.0.0)
  elseif ("${OPENSSL_VERSION}" VERSION_LESS "1.2")
    # OpenSSL 1.1 was added in bionic:
    set(DOCKER_IMAGE_BASE ubuntu:bionic)
    set(DOCKER_IMAGE_PACKAGES libssl1.1)
  else()
    message(WARNING "Unsupported OpenSSL version (${OPENSSL_VERSION})")
  endif()

  message(STATUS "${name} docker image will be based on '${DOCKER_IMAGE_BASE}' "
                 "with '${DOCKER_IMAGE_PACKAGES}' package(s)")

  # This copies deploy/Dockerfile.in to Dockerfile (in current binary directory),
  # replacing DOCKER_IMAGE_BASE and DOCKER_IMAGE_PACKAGES with values set above:
  configure_file(deploy/Dockerfile.in Dockerfile @ONLY)

  # This is needed, because configure_file does not mark output files as 'GENERATED'.
  set_source_files_properties(Dockerfile PROPERTIES GENERATED TRUE)
endmacro()
