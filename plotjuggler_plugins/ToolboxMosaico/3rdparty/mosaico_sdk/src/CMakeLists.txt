set(SDK_SOURCES
    flight/metadata.cpp
    flight/connection_pool.cpp
    flight/query.cpp
    flight/mosaico_client.cpp
)

add_library(mosaico_sdk STATIC ${SDK_SOURCES})

target_include_directories(mosaico_sdk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Plugins always link Arrow Flight statically.
if(NOT TARGET ArrowFlight::arrow_flight_static)
    message(FATAL_ERROR "[MosaicoSDK] ArrowFlight::arrow_flight_static target not found.")
endif()

target_link_libraries(mosaico_sdk PUBLIC
    arrow::arrow
    ArrowFlight::arrow_flight_static
    nlohmann_json::nlohmann_json
)

target_compile_features(mosaico_sdk PUBLIC cxx_std_17)
