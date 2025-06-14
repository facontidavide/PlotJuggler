if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dds_parser")
    set(DDS_PARSER_BUILD_TESTING OFF CACHE BOOL "Force DDS_PARSER_BUILD_TESTING to OFF" FORCE)
    add_subdirectory(dds_parser)
else()
    message(WARNING "dds_parser not found in ${CMAKE_CURRENT_SOURCE_DIR}/dds_parser, skipping!")
    return()
endif()


add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)


add_library(ParserDDS SHARED
    idl_parser.cpp)

target_include_directories(ParserDDS PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/dds_parser/include>
    )

target_link_libraries(ParserDDS
    ${Qt5Widgets_LIBRARIES}
    dds_parser
    plotjuggler_base)

install(TARGETS ParserDDS
    DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
