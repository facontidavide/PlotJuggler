include_directories(../)

# Include Howard Hinnant's date library for improved timestamp parsing
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/date-3.0.4/include)

qt5_wrap_ui(UI_SRC dataload_csv.ui datetimehelp.ui)

set(SRC
    dataload_csv.cpp
    datetimehelp.cpp
    timestamp_parsing.cpp)

add_library(DataLoadCSV SHARED ${SRC} ${UI_SRC})
target_link_libraries(DataLoadCSV PRIVATE Qt5::Widgets Qt5::Xml
                                          plotjuggler_base QCodeEditor)

target_compile_definitions(DataLoadCSV PRIVATE QT_PLUGIN)

install(TARGETS DataLoadCSV DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
