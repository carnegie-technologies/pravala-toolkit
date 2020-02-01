
# Implementation files for all protocol files:

set(PROTO_DIR ${PROJECT_SOURCE_DIR}/proto)

### Basic network types

easy_proto_autogen(network ${PROTO_DIR})
add_library(LibAutoNetwork _network.cpp)
target_link_libraries(LibAutoNetwork LibProto)

### Error codes

easy_proto_autogen(error ${PROTO_DIR})
add_library(LibAutoError _error.cpp)
target_link_libraries(LibAutoError LibProto)

### Log protocol

easy_proto_autogen(log ${PROTO_DIR})
add_library(LibAutoLog _log.cpp)
target_link_libraries(LibAutoLog LibAutoError)

### Control protocol

easy_proto_autogen(ctrl ${PROTO_DIR})
add_library(LibAutoCtrl _ctrl.cpp)
target_link_libraries(LibAutoCtrl LibAutoLog)
