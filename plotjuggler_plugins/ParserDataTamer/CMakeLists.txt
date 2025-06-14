
add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)


add_library(ParserDataTamer SHARED
    datatamer_parser.cpp
    datatamer_parser.h )

target_link_libraries(ParserDataTamer
    ${Qt5Widgets_LIBRARIES}
    plotjuggler_base
    data_tamer_parser
    )


install(TARGETS ParserDataTamer
    DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
