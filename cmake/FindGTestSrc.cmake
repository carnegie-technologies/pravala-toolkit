#
# Locates the GoogleTest source directory
#

find_path(GTEST_SRC_DIR NAMES CMakeLists.txt
  PATHS /usr/local/src/gtest
        /usr/local/src/googletest-read-only
        /usr/src/gtest
        /usr/src/googletest-read-only )

if (GTEST_SRC_DIR)

  # We need to call this first with NO_CMAKE_SYSTEM_PATH,
  # because we prefer it to use the ${GTEST_SRC_DIR}/include (and modifications)
  # (otherwise it would find the system version first)
  # The version with ../.. searches for 'include' at the same level as the source dir
  # (so usr/local/include if usr/local/src/gtest was used, and usr/include if usr/src/gtest was used)
  find_path(GTEST_INCLUDE_DIR NAMES gtest/gtest.h
    PATHS "${GTEST_SRC_DIR}/include"
          "${GTEST_SRC_DIR}/../../include"
          /usr/local/include
          /usr/include
    NO_CMAKE_SYSTEM_PATH )

  # If the above find_path succeeded, this one will NOT be executed:
  find_path(GTEST_INCLUDE_DIR NAMES gtest/gtest.h)

  if (GTEST_INCLUDE_DIR)
    set(GTEST_SRC_FOUND TRUE)
  endif (GTEST_INCLUDE_DIR)

endif (GTEST_SRC_DIR)
