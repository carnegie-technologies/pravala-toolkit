

if (NOT TARGET LibSqlite)
  return()
endif()

include_directories(${3RDPARTY_DIR})

file(GLOB LibDatabase_SRC *.cpp)
add_library(LibDatabase ${LibDatabase_SRC})
target_link_libraries(LibDatabase LibSqlite LibLog)
