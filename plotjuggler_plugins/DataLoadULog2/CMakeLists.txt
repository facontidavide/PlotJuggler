include_directories(../)

find_package(ulog_cpp QUIET)
if(NOT ulog_cpp_FOUND AND NOT TARGET ulog_cpp::ulog_cpp)
  add_subdirectory(3rdparty/ulog_cpp EXCLUDE_FROM_ALL)
endif()

qt5_wrap_ui(UI_SRC ../selectlistdialog.ui ulog_parameters_dialog.ui)
add_library(DataLoadULog2 SHARED
    dataload_ulog2.cpp
    ulog_parameters_dialog.cpp
    ${UI_SRC})
target_link_libraries(DataLoadULog2 PRIVATE ulog_cpp::ulog_cpp
                                             Qt5::Widgets Qt5::Xml
                                             plotjuggler_base)
target_compile_definitions(DataLoadULog2 PRIVATE QT_PLUGIN)
install(TARGETS DataLoadULog2 DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
