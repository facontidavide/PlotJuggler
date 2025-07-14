if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dds-parser/CMakeLists.txt")

  set(DDS_PARSER_BUILD_TESTING
      OFF
      CACHE BOOL "Force DDS_PARSER_BUILD_TESTING to OFF" FORCE)

  set(DDS_PARSER_CSV_EXPORTER
      OFF
      CACHE BOOL "Force DDS_PARSER_CSV_EXPORTER to OFF" FORCE)

  add_subdirectory(dds-parser)

  add_library(ParserDDS SHARED idl_parser.cpp)

  target_include_directories(
    ParserDDS
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/dds_parser/include>)

  target_link_libraries(ParserDDS PRIVATE Qt5::Widgets dds_parser
                                          plotjuggler_base)

  target_compile_definitions(ParserDDS PRIVATE QT_PLUGIN)

  install(TARGETS ParserDDS DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})

endif()
