
include_directories(./utils)

file(GLOB WpaCtrl_SRC utils/base64.c utils/common.c utils/ip_addr.c utils/radiotap.c
    utils/trace.c utils/uuid.c utils/wpa_debug.c utils/wpabuf.c utils/os_unix.c utils/eloop.c
    utils/edit.c common/wpa_ctrl.c)

add_definitions(-DCONFIG_CTRL_IFACE -DCONFIG_CTRL_IFACE_UNIX)

if ( ${SYSTEM_TYPE} STREQUAL "QNX" )
    if ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(mips|ppc|powerpc)" )
        add_definitions(-D__BYTE_ORDER=__BIG_ENDIAN)
    elseif ( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(x86|arm|aarch64|sh)" )
        add_definitions(-D__BYTE_ORDER=__LITTLE_ENDIAN)
    else()
        message ( FATAL_ERROR "Cannot determine byte order" )
    endif()
endif()

add_library(LibWpaCtrl ${WpaCtrl_SRC})
