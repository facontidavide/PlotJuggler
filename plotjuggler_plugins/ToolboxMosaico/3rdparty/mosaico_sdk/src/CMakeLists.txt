set(SDK_SOURCES
    mcap_impl.cpp
    mcap_reader.cpp
    decoders/decoder.cpp
    decoders/decoder_factory.cpp
    decoders/json_decoder.cpp
    decoders/passthrough_decoder.cpp
    decoders/msg_parser.cpp
    ontology/ontology_registry.cpp
    ontology/tag_resolver.cpp
    ontology/builders/builder_utils.cpp
    ontology/builders/geometry_builders.cpp
    ontology/builders/compound_geometry_builders.cpp
    ontology/builders/imu_builder.cpp
    ontology/builders/gps_builders.cpp
    ontology/builders/nav_builders.cpp
    ontology/builders/sensor_builders.cpp
    ontology/builders/std_type_builder.cpp
    flight/metadata.cpp
    flight/connection_pool.cpp
    flight/query.cpp
    flight/upload_session.cpp
    flight/mosaico_client.cpp
    flight/upload_mcap.cpp
)

if(WITH_CDR)
    # CDR decoding uses the header-only nanocdr (provided by the parent
    # project's include path). No external library to link against.
    list(APPEND SDK_SOURCES decoders/cdr_decoder.cpp)
endif()

if(WITH_ROS1)
    list(APPEND SDK_SOURCES decoders/ros1_decoder.cpp)
endif()

if(WITH_PROTOBUF)
    find_package(protobuf QUIET)
    if(protobuf_FOUND)
        list(APPEND SDK_SOURCES decoders/protobuf_decoder.cpp)
    else()
        message(STATUS "[mosaico_sdk] protobuf not found — Protobuf decoder disabled")
        set(WITH_PROTOBUF OFF)
    endif()
endif()

add_library(mosaico_sdk STATIC ${SDK_SOURCES})

target_include_directories(mosaico_sdk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(mosaico_sdk PUBLIC
    arrow::arrow
    nlohmann_json::nlohmann_json
)

target_compile_definitions(mosaico_sdk PUBLIC
    MCAP_COMPRESSION_NO_LZ4
    MCAP_COMPRESSION_NO_ZSTD
)

if(WITH_CDR)
    target_compile_definitions(mosaico_sdk PUBLIC MOSAICO_HAS_CDR)
endif()

if(WITH_ROS1)
    target_compile_definitions(mosaico_sdk PUBLIC MOSAICO_HAS_ROS1)
endif()

if(WITH_PROTOBUF)
    target_compile_definitions(mosaico_sdk PUBLIC MOSAICO_HAS_PROTOBUF)
    target_link_libraries(mosaico_sdk PUBLIC protobuf::libprotobuf)
    target_include_directories(mosaico_sdk SYSTEM BEFORE PUBLIC ${CMAKE_INCLUDE_PATH})
endif()

target_compile_features(mosaico_sdk PUBLIC cxx_std_17)
