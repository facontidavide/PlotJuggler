
include_directories( ../ )

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

find_package(Protobuf QUIET)

if( Protobuf_FOUND)
    message(STATUS "[Protobuf] found")
    QT5_WRAP_UI ( UI_SRC
        protobuf_parser.ui
        )

    include_directories(${Protobuf_INCLUDE_DIRS})

    add_library(ProtobufParser SHARED
        error_collectors.cpp
        protobuf_parser.cpp
        protobuf_factory.cpp
        protobuf_parser.h
        ${UI_SRC}  )

    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        find_library(ABSL_SPINLOCK_WAIT_LIB absl_spinlock_wait)

        target_link_libraries(ProtobufParser ${ABSL_SPINLOCK_WAIT_LIB} )
    endif()

    target_link_libraries(ProtobufParser
        ${Qt5Widgets_LIBRARIES}
        ${Qt5Xml_LIBRARIES}
        ${Protobuf_LIBRARIES}
        plotjuggler_base
        plotjuggler_qwt)

    install(TARGETS ProtobufParser DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
else()
    message("[Protobuf] not found: skipping compilatiopn of plugin ProtobufParser")
endif()
