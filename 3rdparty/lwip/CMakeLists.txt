
#
# This file adds 'lwip' library, built using lwIP sources.
# To use this library, uint32_t sys_now() must be implemented elsewhere.
# sys_now function should return the current time-stamp in milliseconds
# (it doesn't need to represent calendar time and we don't care for wraparound, it is only used for time diffs).
#

include(TestBigEndian)

TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

if(${IS_BIG_ENDIAN})
    message(FATAL_ERROR "lwIP is currently not configured for big-endian architectures")
endif()

set(LWIP_ORG_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/lwip)
set(LWIP_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/lwip)

set(OPTS_HDR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lwipopts.h)
set(OPTS_HDR_DST ${LWIP_BUILD_DIR}/src/include/lwipopts.h)

set(ARCH_HDR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/arch_cc.h)
set(ARCH_HDR_DST ${LWIP_BUILD_DIR}/src/include/arch/cc.h)

set(TAGS_PATCH          ${CMAKE_CURRENT_SOURCE_DIR}/lwip-tags.patch)
set(TCP_COUNTS_PATCH    ${CMAKE_CURRENT_SOURCE_DIR}/lwip-tcp-counts.patch)

# We need to modify some lwip source files, so we copy the directory to inside of the build directory,
# and modify them there. Otherwise (if we modified the originals) we would have issues updating lwip submodule.

# List of original lwip source files
file(GLOB Lwip_ORG_SRC ${LWIP_ORG_DIR}/src/core/*.c ${LWIP_ORG_DIR}/src/core/*/*.c ${LWIP_ORG_DIR}/src/api/err.c)

# We need a list of the same files in the build directory (once we copy them).
# We can't simply use file(GLOB ...) for them, because they don't exist yet.
# So we create a new list, with the LWIP_ORG_DIR replaced with LWIP_BUILD_DIR.

unset(Lwip_SRC)

string(LENGTH "${LWIP_ORG_DIR}/" LWIP_ORG_DIR_LENGTH)

foreach(file ${Lwip_ORG_SRC})
  string(SUBSTRING ${file} ${LWIP_ORG_DIR_LENGTH} -1 src_file)
  set(Lwip_SRC ${Lwip_SRC} "${LWIP_BUILD_DIR}/${src_file}")
endforeach(file)

# Command that (re)generates the build directory.
# We only copy 'lwip/src'. We don't need anything else,
# and we do NOT want .git file in there (causes problems with git).

add_custom_command(OUTPUT ${OPTS_HDR_DST} ${ARCH_HDR_DST} ${Lwip_SRC}
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${LWIP_BUILD_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${LWIP_ORG_DIR}/src ${LWIP_BUILD_DIR}/src
  COMMAND patch -p1 -d ${LWIP_BUILD_DIR} -i ${TAGS_PATCH}
  COMMAND patch -p1 -d ${LWIP_BUILD_DIR} -i ${TCP_COUNTS_PATCH}
  COMMAND ${CMAKE_COMMAND} -E copy ${OPTS_HDR_SRC} ${OPTS_HDR_DST}
  COMMAND ${CMAKE_COMMAND} -E copy ${ARCH_HDR_SRC} ${ARCH_HDR_DST}
  DEPENDS ${OPTS_HDR_SRC} ${ARCH_HDR_SRC} ${TAGS_PATCH} ${Lwip_ORG_SRC}
)

include_directories(${LWIP_BUILD_DIR}/src/include)

# Ignore warnings:
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format")

add_library(lwip ${Lwip_SRC})

# Logging support on Android
if(${SYSTEM_TYPE} STREQUAL "Android")
  target_link_libraries(lwip log)
endif()
