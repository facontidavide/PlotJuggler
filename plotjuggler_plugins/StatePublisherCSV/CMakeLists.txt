include_directories(../)

qt5_wrap_ui(UI_SRC publisher_csv_dialog.ui)

add_library(PublisherCSV SHARED publisher_csv.cpp ${UI_SRC})

target_link_libraries(PublisherCSV PRIVATE Qt5::Widgets plotjuggler_base)

target_compile_definitions(PublisherCSV PRIVATE QT_PLUGIN)

install(TARGETS PublisherCSV DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
