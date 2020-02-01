
file(GLOB SQLite_SRC sqlite3.c)
add_definitions(-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION)
add_library(LibSqlite ${SQLite_SRC})
