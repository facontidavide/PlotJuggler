add_library(ParserDataTamer SHARED datatamer_parser.cpp datatamer_parser.h)

target_link_libraries(ParserDataTamer PRIVATE Qt5::Widgets plotjuggler_base
                                              data_tamer_parser fmt::fmt)

target_compile_definitions(ParserDataTamer PRIVATE QT_PLUGIN)

install(TARGETS ParserDataTamer DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
