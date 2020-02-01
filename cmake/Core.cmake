
if (CORE_CMAKE_MODULE_PROCESSED)
  return()
endif()

set (CORE_CMAKE_MODULE_PROCESSED 1)

# Core module. It sets various variables to be used by projects.
# Variables defined in CMakeLists.txt files of each project are
# invisible to higher projects. Variables defined in
# this modules - are (in each project that includes this module)

cmake_minimum_required(VERSION 3.5)

include(FindPkgConfig)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(CheckSymbolExists)
include(CMakePushCheckState)

if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif()

if (NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
  message ( FATAL_ERROR "CMAKE_SYSTEM_PROCESSOR is not set" )
endif()

message(STATUS "Using CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Using CMAKE_CROSSCOMPILING=${CMAKE_CROSSCOMPILING}")

# We typically don't use 'make install' which normally gets rid of the rpath.
# So we just tell cmake not to add it at all:
set(CMAKE_SKIP_RPATH TRUE)

set(ECFG_OPTIONS ${ECFG_OPTIONS} -t "cpu-${CMAKE_SYSTEM_PROCESSOR}")

if(NOT MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

if (CMAKE_CROSSCOMPILING)
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -t "cross")
endif()

if (VOIP_CLIENT)
    set(CLIENT_ECFG_TAGS ${CLIENT_ECFG_TAGS} -t "voip-client")
endif()

if (${UNIX})

  # Depending on how we are built (using Android's Gradle, or our own cross building),
  # we may get SYSTEM_TYPE=Android and CMAKE_SYSTEM_NAME=Linux, or just CMAKE_SYSTEM_NAME=Android.
  # If it's the latter, we need to configure SYSTEM_TYPE.

  if (NOT SYSTEM_TYPE AND ${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(SYSTEM_TYPE "Android")
  endif()

  if ("${SYSTEM_TYPE}" STREQUAL "Android")

    add_definitions(-DSYSTEM_LINUX=1 -DANDROID=1 -DPLATFORM_ANDROID=1)
    set(ECFG_OPTIONS ${ECFG_OPTIONS} -t linux -t android)

    if(NOT PREBUILT_SYSTEM_TYPE)
      set(PREBUILT_SYSTEM_TYPE "Android")
    endif()

  elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_definitions(-DSYSTEM_LINUX=1)

    if (NOT SYSTEM_TYPE)
      set(SYSTEM_TYPE "Linux")
    endif()

    if (PLATFORM_OPENWRT)
      add_definitions(-DPLATFORM_OPENWRT=1 -DHAVE_POSIX_FALLOCATE=0)
      message(STATUS "Enabling OpenWRT build mode")
      set(ECFG_OPTIONS ${ECFG_OPTIONS} -t linux -t openwrt)
    else()
      set(ECFG_OPTIONS ${ECFG_OPTIONS} -t linux)
    endif()
  elseif (${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    add_definitions(-DSYSTEM_UNIX=1)
    set(SYSTEM_TYPE "BSD")
    set(CMAKE_CXX_COMPILER eg++)
    set(ECFG_OPTIONS ${ECFG_OPTIONS} -t openbsd)
  elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    add_definitions(-DSYSTEM_UNIX=1 -DSYSTEM_APPLE=1)
    set(SYSTEM_TYPE "Apple")
    if (PLATFORM_IOS)
      message(STATUS "Building for iOS (arch: ${CMAKE_SYSTEM_PROCESSOR})")
      set(ECFG_OPTIONS ${ECFG_OPTIONS} -t ios)
      add_definitions(-DPLATFORM_IOS=1)
      if(NOT PREBUILT_SYSTEM_TYPE)
        set(PREBUILT_SYSTEM_TYPE "iOS")
      endif()
    else()
      set(ECFG_OPTIONS ${ECFG_OPTIONS} -t darwin)
    endif()
  elseif (${CMAKE_SYSTEM_NAME} STREQUAL "QNX")
    set(SYSTEM_TYPE "QNX")

    add_definitions(-DSYSTEM_UNIX=1 -DSYSTEM_QNX=1)

    # QNX is unix enough, but its compiler doesn't define it
    add_definitions(-Dunix=1 -D__unix=1 -D__unix__=1)

    # QNX needs this defined to enable POSIX/BSD "extensions" (e.g. stat, PATH_MAX, strlcpy)
    add_definitions(-D__EXT=1)

    # QNX doesn't support C9X by default (past C89 language features), so add to support isnan and isinf
    add_definitions(-D_HAS_C9X=1)

    set(ECFG_OPTIONS ${ECFG_OPTIONS} -t qnx)

    # The compiler must be "nto<ARCH>-gcc" since the default cc setups up the env wrong
    if (NOT ${CMAKE_C_COMPILER} MATCHES "/nto(.*)-gcc$")
      message (WARNING "This will probably not work, compiler should be something like 'nto${CMAKE_SYSTEM_PROCESSOR}-gcc', try using CC=nto${CMAKE_SYSTEM_PROCESSOR}-gcc")
    endif()

    # The compiler must be "nto<ARCH>-g++" since the default CC setups up the env wrong
    if (NOT ${CMAKE_CXX_COMPILER} MATCHES "/nto(.*)-g\\+\\+$")
      message (WARNING "This will probably not work, compiler should be something like 'nto${CMAKE_SYSTEM_PROCESSOR}-g++', try using CXX=nto${CMAKE_SYSTEM_PROCESSOR}-g++")
    endif()

    # QNX has broken C++ threadsafe static object creation after fork.
    # Since we don't use threads on QNX anyways, we can just disable this.
    # This is NOT needed for C code (since C doesn't have objects).
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics")

  endif()
elseif (${WIN32})
  add_definitions(-DSYSTEM_WINDOWS=1 -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
  set(SYSTEM_TYPE "Windows")
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -t windows)
endif()

if (NOT SYSTEM_TYPE)
  set(SYSTEM_TYPE "Unknown")
endif()

message(STATUS "System type is set to '${SYSTEM_TYPE}'")

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i.86" AND NOT CPU_MARCH)
  set(CPU_MARCH i586)
endif()

if (CPU_MARCH)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${CPU_MARCH}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${CPU_MARCH}")
else()
  # We want SSE 4.2 auto-detection only when NOT forcing CPU_MARCH.
  # We also don't want to do that if we're cross compiling or on Windows:

  if (NOT CMAKE_CROSSCOMPILING AND NOT WIN32)
    include(CheckCXXCompilerFlag)

    CHECK_CXX_COMPILER_FLAG(-msse4.2 HAS_SSE_42)
    if (HAS_SSE_42)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2")
    endif()
  endif()
endif()

if (ENABLE_PROFILING)
  message (STATUS "Enabling profiling with gprof")
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -no-pie -pg")
  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -no-pie -pg")
  set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} -no-pie -pg")
  set(CMAKE_EXE_FLAGS  "${CMAKE_EXE_FLAGS} -no-pie -pg")
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -t profiling)
endif()

if (ENABLE_FRAME_POINTER)
  message (STATUS "Enabling frame pointer")
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
  set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} -fno-omit-frame-pointer")
  set(CMAKE_EXE_FLAGS  "${CMAKE_EXE_FLAGS} -fno-omit-frame-pointer")
endif()

if (ENABLE_PERFTOOLS)
  message (STATUS "Enabling Google PerfTools")
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -ltcmalloc")
  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -ltcmalloc")
  set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} -ltcmalloc")
  set(CMAKE_EXE_FLAGS  "${CMAKE_EXE_FLAGS} -ltcmalloc")
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -t perftools)
endif()

if (NOT CMAKE_BUILD_TYPE)
  message (STATUS "Setting build type to Debug")
  set (CMAKE_BUILD_TYPE "Debug")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Build type: DEBUG")
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -t debug)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Build type: RELEASE")
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -t release)

  if(NOT MSVC)
    if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
      # Clang doesn't support '-s' option.
      set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -s")
      set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -s")
    endif()

    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fvisibility=hidden")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

    if( NOT ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
      # Not used on OpenBSD:
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
    endif()
  endif()
endif()

# Let's set the build type for EcfgGen:
set(ECFG_OPTIONS ${ECFG_OPTIONS} -b "${CMAKE_BUILD_TYPE}")

if(NOT DEFINED ENABLE_VHOSTNET AND ${SYSTEM_TYPE} STREQUAL "Linux")
  # Enable VHOST support by default on Linux, unless we are building with Clang.
  # Clang doesn't like the code used in virtio_ring.h.
  # With GCC we need to enable -fpermissive for the source file that uses that header,
  # to allow casting void pointer to struct vring_desc, and to allow for void pointer arithmetic (GCC extension).
  # Clang doesn't support that.
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    # This warning only applies to automatically-enabled vhost-net support.
    # If someone passes -DENABLE_VHOSTNET to cmake, they will get a build error.
    message(WARNING "vhost-net support cannot be enabled while building with Clang")
  else()
    set(ENABLE_VHOSTNET true)
  endif()
endif()

if (ENABLE_VHOSTNET)
  message(STATUS "vhost-net support enabled")
  add_definitions(-DENABLE_VHOSTNET=1)
else()
  message(STATUS "vhost-net support disabled")
endif()

cmake_push_check_state(RESET)

set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE=1")
set(CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")

check_symbol_exists(recvmmsg "sys/socket.h" HAVE_RECVMMSG)
check_symbol_exists(sendmmsg "sys/socket.h" HAVE_SENDMMSG)
check_symbol_exists(malloc_usable_size "malloc.h" HAVE_MALLOC_USABLE_SIZE)
check_type_size("struct mmsghdr" HAVE_MMSGHDR)

cmake_pop_check_state()

if(HAVE_RECVMMSG)
  add_definitions(-DHAVE_RECVMMSG=1)
endif()

if(HAVE_SENDMMSG)
  add_definitions(-DHAVE_SENDMMSG=1)
endif()

if(HAVE_MALLOC_USABLE_SIZE)
  add_definitions(-DHAVE_MALLOC_USABLE_SIZE=1)
endif()

if(HAVE_MMSGHDR)
  add_definitions(-DHAVE_MMSGHDR=1)
endif()

if(DISABLE_LOGGING)
  add_definitions(-DNO_LOGGING=1)
  message(STATUS "All log messages will be DISABLED in this build")
endif()

if(NOT PREBUILT_SYSTEM_TYPE)
  set(PREBUILT_SYSTEM_TYPE "${SYSTEM_TYPE}")
endif()

if (NOT PREBUILT_ARCH_DIR)
  if("${ANDROID_ABI}" STREQUAL "")
    set(PREBUILT_ARCH_DIR "${CMAKE_SYSTEM_PROCESSOR}")
  else()
    set(PREBUILT_ARCH_DIR "${ANDROID_ABI}")
  endif()
endif()

if (NOT PREBUILT_ROOT)
  # We want to set this only if it wasn't set before (for example by the cross build script)
  set(PREBUILT_ROOT "${PROJECT_SOURCE_DIR}/prebuilt/${PREBUILT_SYSTEM_TYPE}")

  if (${PREBUILT_ARCH_DIR} MATCHES "^(i386|i686)$")
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      # i386 is the proper name for x86 on Apple platforms
      set(PREBUILT_ARCH_DIR "i386")
    else()
      set(PREBUILT_ARCH_DIR "x86")
    endif()
  endif()
  if (IS_DIRECTORY "${PREBUILT_ROOT}/${PREBUILT_ARCH_DIR}")
    set(PREBUILT_ROOT "${PREBUILT_ROOT}/${PREBUILT_ARCH_DIR}")
  endif()
endif()

message(STATUS "Using PREBUILT_ROOT: ${PREBUILT_ROOT}")

# We don't need zlib on Windows, and linking it in results in things like EcfgGen and ProtoGen
# expecting zlib1.dll to be available in its directory. So we just won't bother looking for zlib
# on Windows.
if (NOT ZLIB_FOUND AND NOT ${SYSTEM_TYPE} STREQUAL "Windows")
  if (EXISTS "${PREBUILT_ROOT}/include/zlib.h")
    set(ZLIB_FOUND true)
    set(ZLIB_INCLUDE_DIRS "${PREBUILT_ROOT}/include")
    set(ZLIB_LIBRARIES "-L${PREBUILT_ROOT}/lib -lz")
  else()
    find_package(ZLIB)
  endif()
endif()

if (ZLIB_FOUND)
  message(STATUS "Using ZLIB includes: ${ZLIB_INCLUDE_DIRS}")
  message(STATUS "Using ZLIB libraries: '${ZLIB_LIBRARIES}'")
  add_definitions(-DHAVE_ZLIB=1)
endif()

if (NOT OPENSSL_FOUND)
  if (EXISTS "${PREBUILT_ROOT}/include/openssl/ssl.h")
    set(OPENSSL_FOUND true)
    set(OPENSSL_INCLUDE_DIR "${PREBUILT_ROOT}/include")

    if (${SYSTEM_TYPE} STREQUAL "Windows")
      set(OPENSSL_LIBRARIES "${PREBUILT_ROOT}/lib/libeay32.lib;${PREBUILT_ROOT}/lib/ssleay32.lib")
    else()
      set(OPENSSL_LIBRARIES "-L${PREBUILT_ROOT}/lib -lssl -lcrypto")
    endif()
  else()
    find_package(OpenSSL)
  endif()
endif()

if (OPENSSL_FOUND)
  message(STATUS "Using SSL includes: ${OPENSSL_INCLUDE_DIR}")
  message(STATUS "Using SSL libraries: '${OPENSSL_LIBRARIES}'")
endif()

# searching for libcurl can be disabled, e.g. for cross-compiling without pkg-config support
if (NOT LIBCURL_FOUND)
  if (EXISTS "${PREBUILT_ROOT}/include/curl/curl.h")
    set(LIBCURL_FOUND true)
    set(LIBCURL_INCLUDE_DIRS "${PREBUILT_ROOT}/include")
    if (${SYSTEM_TYPE} STREQUAL "Windows")
      set(LIBCURL_DEFINES "-DCURL_STATICLIB")
      set(LIBCURL_LDFLAGS "${PREBUILT_ROOT}/lib/libcurl.lib;${PREBUILT_ROOT}/lib/zlib.lib")
    elseif(${SYSTEM_TYPE} STREQUAL "Android")
      set(LIBCURL_LDFLAGS "-L${PREBUILT_ROOT}/lib -lcurl")
      set(LIBMBEDTLS_LDFLAGS "-L${PREBUILT_ROOT}/lib -lmbedtls")
    elseif(PLATFORM_IOS)
      set(LIBCURL_LDFLAGS "-L${PREBUILT_ROOT}/lib -lcurl -lcares")
      set(LIBMBEDTLS_LDFLAGS "-L${PREBUILT_ROOT}/lib -lmbedtls -lmbedcrypto -lmbedx509")
    else()
      set(LIBCURL_LDFLAGS "-L${PREBUILT_ROOT}/lib -lcurl -lcares")
    endif()
  elseif (${SYSTEM_TYPE} STREQUAL "Android")
    # We can't rely on pkgconfig for Android. It would find our host system's curl.
    # If we use CMake's find_package everything is OK, because it respects CMAKE_FIND_ROOT_PATH
    # toolchain's option. However, pkg_search_module does not.
    # Also, we can't disable it for ALL cross builds, because it works fine if PKG_CONFIG_PATH is provided.
    # On Android we don't have pkgconfig and we don't need cUrl, so we just disable it on this specific platform.
    #
    # The other solution would be to add FindCurl cmake module and never use pkg_search_module, but it's more work...
  else()
    pkg_search_module(LIBCURL libcurl)
  endif()
endif()

if (LIBCURL_FOUND)
  message(STATUS "Using CURL include dirs: ${LIBCURL_INCLUDE_DIRS}")
  message(STATUS "Using CURL LDFLAGS: '${LIBCURL_LDFLAGS}'")
endif()

if (NOT DBUS_FOUND)
  pkg_search_module(DBUS dbus-1)
endif()

if (DBUS_FOUND)
  message(STATUS "Using DBUS include dirs: ${DBUS_INCLUDE_DIRS}")
  message(STATUS "Using DBUS LDFLAGS: '${DBUS_LDFLAGS}'")
endif()

if (ENABLE_LIBEVENT)

  message (STATUS "Enabling LibEvent")
  find_package(LibEvent REQUIRED)
  add_definitions(-DUSE_LIBEVENT=1)
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -t libevent)

else()

  if (ENABLE_POLL)
    check_include_files ("poll.h" HAVE_POLL_H)

    if(${HAVE_POLL_H})
        message (STATUS "Enabling Poll")
        add_definitions(-DUSE_POLL=1)
        set(ECFG_OPTIONS ${ECFG_OPTIONS} -t poll)
    else()
        message(FATAL_ERROR "Could not find poll.h")
    endif(${HAVE_POLL_H})
  endif()

  if(NOT DISABLE_SIGNALFD AND NOT ${SYSTEM_TYPE} STREQUAL "Windows")
      check_symbol_exists(signalfd "sys/signalfd.h" HAVE_SIGNALFD)
  endif()

  if(${HAVE_SIGNALFD})
    message(STATUS "Signal handling: signalfd")
    add_definitions(-DUSE_SIGNALFD=1)
  else()
    message(STATUS "Signal handling: legacy")
  endif()

endif()

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused -Wcast-qual -Werror -Wno-long-long -Wno-variadic-macros -fno-rtti -fno-exceptions")
  # Disabled: -Wconversion -Wsign-conversion (-pedantic, -Wshadow disabled for tests, enabled for rest)

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    # We need -Wno-gnu-folding-constant to avoid this warning:
    # "variable length array folded to constant array as an extension"
    # in this type of situation:
    #   char control[ CMSG_SPACE ( sizeof ( int ) ) ]
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-long-long -Wno-gnu-folding-constant")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi")
  endif()

else()
  # Warning we want to see (and treat as errors):
  # C4239 - creating non-const reference to a temporary object - this is an MSVC extension, which we DON'T want.
  # MP - enable source file level parallel compiles
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /we4239 /MP")
endif()

if (STATIC_BINARIES)
  message (STATUS "Binaries will use STATIC linking")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -t static)
  if (LIBSSL_LDFLAGS)
    # When static linking is used, OpenSSL requires the 'dl' library to be added
    set(LIBSSL_LDFLAGS ${LIBSSL_LDFLAGS} dl)
  endif()
else()
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -t dynamic)
endif()

if(NOT DEFINED REVISION_SUFFIX)
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -r custom)
elseif(NOT ${REVISION_SUFFIX} STREQUAL "")
  set(ECFG_OPTIONS ${ECFG_OPTIONS} -r "${REVISION_SUFFIX}")
endif()

message (STATUS "Using ECFG options: ${ECFG_OPTIONS}")
