include_directories(../)

qt5_wrap_ui(UI_SRC ../selectlistdialog.ui ulog_parameters_dialog.ui)

set(SRC dataload_ulog.cpp ulog_parser.cpp ulog_parameters_dialog.cpp)

add_library(DataLoadULog SHARED ${SRC} ${UI_SRC})

target_link_libraries(DataLoadULog PRIVATE Qt5::Widgets Qt5::Xml
                                           plotjuggler_base)

target_compile_definitions(DataLoadULog PRIVATE QT_PLUGIN)

install(TARGETS DataLoadULog DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
