find_package(Arrow QUIET CONFIG)
find_package(ArrowFlight QUIET CONFIG)
if(NOT Arrow_FOUND OR NOT ArrowFlight_FOUND)
    message("[ToolboxMosaico] Arrow/Flight not found. Skipping plugin.")
    return()
endif()

# The SDK links arrow::arrow (Conan convention). PlotJuggler provides
# Arrow::arrow_shared + ArrowFlight::arrow_flight_shared. Bridge them.
if(NOT TARGET arrow::arrow)
    add_library(arrow_combined INTERFACE)
    target_link_libraries(arrow_combined INTERFACE
        Arrow::arrow_shared
        ArrowFlight::arrow_flight_shared)
    add_library(arrow::arrow ALIAS arrow_combined)
endif()

# Vendored Mosaico SDK. Lives at 3rdparty/mosaico_sdk/.
# Built as a static library and linked into the plugin.
set(BUILD_TESTING_SAVED ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/mosaico_sdk EXCLUDE_FROM_ALL)
set(BUILD_TESTING ${BUILD_TESTING_SAVED} CACHE BOOL "" FORCE)

include_directories(../)

set(MOSAICO_SOURCES
    src/core/time_format.cpp
    src/ui/main_window.cpp
    src/ui/fetch_worker.cpp
    src/ui/server_history.cpp
    src/ui/sequence/sequence_panel.cpp
    src/ui/topic/topic_panel.cpp
    src/ui/metadata/data_view_panel.cpp
    src/ui/picker/calendar_widget.cpp
    src/ui/picker/dual_calendar_widget.cpp
    src/ui/picker/time_picker_widget.cpp
    src/ui/picker/sequence_picker_widget.cpp
    src/ui/metadata/query_bar.cpp
    src/ui/metadata/completer.cpp
    src/ui/security/cert_dialog.cpp
    toolbox_mosaico.cpp
)

qt5_add_resources(MOSAICO_RESOURCES resources/mosaico_resources.qrc)

add_library(ToolboxMosaico SHARED ${MOSAICO_SOURCES} ${MOSAICO_RESOURCES})

target_include_directories(ToolboxMosaico PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/query
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

target_link_libraries(ToolboxMosaico PRIVATE
    Qt5::Widgets
    Qt5::Xml
    plotjuggler_base
    mosaico_sdk
    lua::lua
    QCodeEditor
)

target_compile_definitions(ToolboxMosaico PRIVATE QT_PLUGIN)

install(TARGETS ToolboxMosaico DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})

# Tests (optional — only built when BUILD_TESTING is ON)
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
