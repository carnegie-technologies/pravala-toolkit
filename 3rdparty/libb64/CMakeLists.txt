
include_directories(libb64/include)
file(GLOB LibB64_SRC libb64/src/*.c)
add_library(LibB64 ${LibB64_SRC})

