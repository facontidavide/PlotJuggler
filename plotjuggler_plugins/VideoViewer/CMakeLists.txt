
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(QtAV QUIET)

if(NOT QtAV_FOUND)
    message("QtAV not found. VideoPlayer plugin will not be built. Try installing it with 'sudo apt install libqtav-dev'")
else()
    message("QtAV FOUND. Building VideoPlayer plugin")

    include_directories( ../ ${QTAVWIDGETS_INCLUDE_DIRS})

    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    QT5_WRAP_UI ( UI_SRC  video_dialog.ui  )

    add_library(PublisherVideoViewer SHARED
        video_viewer.cpp
        video_dialog.cpp
        ${UI_SRC}  )

    target_link_libraries(PublisherVideoViewer
        Qt5::Widgets
        ${QTAVWIDGETS_LIBRARIES}
        plotjuggler_base
        )
    install(TARGETS PublisherVideoViewer DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
endif()
