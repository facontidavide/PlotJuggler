include_directories(../)

find_package(ulog_cpp QUIET)
if(NOT ulog_cpp_FOUND AND NOT TARGET ulog_cpp::ulog_cpp)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../DataLoadULog2/3rdparty/ulog_cpp
                   ${CMAKE_CURRENT_BINARY_DIR}/ulog_cpp EXCLUDE_FROM_ALL)
endif()

# Plugin (Qt GUI)
qt5_wrap_ui(UI_SRC ../selectlistdialog.ui ulog_parameters_dialog.ui)
add_library(DataLoadULog SHARED
    dataload_ulog.cpp
    ulog_parser.cpp
    ulog_parameters_dialog.cpp
    ${UI_SRC})
target_link_libraries(DataLoadULog PRIVATE ulog_cpp::ulog_cpp
                                           Qt5::Widgets Qt5::Xml
                                           plotjuggler_base)
target_compile_definitions(DataLoadULog PRIVATE QT_PLUGIN)
install(TARGETS DataLoadULog DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})

# Tests
if(BUILD_TESTING)
  find_package(GTest QUIET)
  if(GTest_FOUND)
    enable_testing()
    add_executable(test_ulog_parser tests/test_ulog_parser.cpp ulog_parser.cpp)
    target_link_libraries(test_ulog_parser PRIVATE ulog_cpp::ulog_cpp GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(test_ulog_parser)
  endif()
endif()
