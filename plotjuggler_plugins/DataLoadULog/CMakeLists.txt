include_directories(../)

# Pure parsing library (no Qt dependency)
add_library(ulog_parser_lib STATIC ulog_parser.cpp)
target_include_directories(ulog_parser_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Plugin (Qt GUI)
qt5_wrap_ui(UI_SRC ../selectlistdialog.ui ulog_parameters_dialog.ui)
add_library(DataLoadULog SHARED dataload_ulog.cpp ulog_parameters_dialog.cpp ${UI_SRC})
target_link_libraries(DataLoadULog PRIVATE ulog_parser_lib
                                           Qt5::Widgets Qt5::Xml
                                           plotjuggler_base)
target_compile_definitions(DataLoadULog PRIVATE QT_PLUGIN)
install(TARGETS DataLoadULog DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})

# Tests
if(BUILD_TESTING)
  find_package(GTest QUIET)
  if(GTest_FOUND)
    enable_testing()
    add_executable(test_ulog_parser tests/test_ulog_parser.cpp)
    target_link_libraries(test_ulog_parser PRIVATE ulog_parser_lib GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(test_ulog_parser)
  endif()
endif()
