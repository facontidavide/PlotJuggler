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

set(MOSAICO_SDK_ARROW_FLIGHT_TARGET "")
foreach(candidate ArrowFlight::arrow_flight_static ArrowFlight::arrow_flight_shared)
    if(TARGET ${candidate})
        set(MOSAICO_SDK_ARROW_FLIGHT_TARGET ${candidate})
        break()
    endif()
endforeach()
if(NOT MOSAICO_SDK_ARROW_FLIGHT_TARGET)
    message(FATAL_ERROR "[MosaicoSDK] Arrow Flight target not found.")
endif()

target_link_libraries(mosaico_sdk PUBLIC
    arrow::arrow
    ${MOSAICO_SDK_ARROW_FLIGHT_TARGET}
    nlohmann_json::nlohmann_json
)

target_compile_features(mosaico_sdk PUBLIC cxx_std_17)
