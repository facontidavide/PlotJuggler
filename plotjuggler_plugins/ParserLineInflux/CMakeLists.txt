add_library(ParserLineInflux SHARED line_parser.cpp line_parser.h)

target_link_libraries(ParserLineInflux PRIVATE Qt5::Widgets plotjuggler_base)

target_compile_definitions(ParserLineInflux PRIVATE QT_PLUGIN)

install(TARGETS ParserLineInflux DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
