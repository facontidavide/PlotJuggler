
include_directories( ../ )

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

message(STATUS "[MAVLink] build")
# QT5_WRAP_UI ( UI_SRC
#     protobuf_parser.ui
#     )

include_directories(
    mavlink
    mavlink/common
)

add_library(MAVLinkParser SHARED
    mavlink_parser.cpp
    mavlink_factory.h
    mavlink_parser.h
    ${UI_SRC}  )

target_link_libraries(MAVLinkParser
    plotjuggler_base
    plotjuggler_qwt)

install(TARGETS MAVLinkParser DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
