include_directories(../)

# Include Howard Hinnant's date library for improved timestamp parsing
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/date-3.0.4/include)

qt5_wrap_ui(UI_SRC dataload_csv.ui datetimehelp.ui)

# Pure parsing library (no Qt Widgets dependency)
add_library(csv_parser_lib STATIC csv_parser.cpp timestamp_parsing.cpp)
target_link_libraries(csv_parser_lib PUBLIC Qt5::Core)
target_include_directories(csv_parser_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/3rdparty/date-3.0.4/include)

# Plugin (GUI)
add_library(DataLoadCSV SHARED dataload_csv.cpp datetimehelp.cpp ${UI_SRC})
target_link_libraries(DataLoadCSV PRIVATE csv_parser_lib Qt5::Widgets Qt5::Xml
                                          plotjuggler_base QCodeEditor)

target_compile_definitions(DataLoadCSV PRIVATE QT_PLUGIN)

install(TARGETS DataLoadCSV DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})

# Tests
if(BUILD_TESTING)
  find_package(GTest QUIET)
  if(GTest_FOUND)
    enable_testing()
    add_executable(test_csv_parser tests/test_csv_parser.cpp)
    target_link_libraries(test_csv_parser PRIVATE csv_parser_lib GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(test_csv_parser)
  endif()
endif()
