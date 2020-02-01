
# Requires OpenSSL for SHA1
include_directories(${3RDPARTY_DIR}/misc ${OPENSSL_INCLUDE_DIR})
file(GLOB LibWebSocket_SRC *.cpp internal/*.cpp)
add_library(LibWebSocket ${LibWebSocket_SRC})
target_link_libraries(LibWebSocket LibNet LibHttp LibBase64 ${OPENSSL_LIBRARIES})
