
file(GLOB ProtoFiles ${PROJECT_SOURCE_DIR}/proto/*/*.proto
                     ${PROJECT_SOURCE_DIR}/proto/tests/*.proto
                     ${PROJECT_SOURCE_DIR}/proto/tests/*/*.proto)

add_custom_target(ParseProtoFiles
    ProtoGen null --id-scope=branch ${ProtoFiles}
    DEPENDS ${ProtoFiles}
  )

add_dependencies(tests ParseProtoFiles)
