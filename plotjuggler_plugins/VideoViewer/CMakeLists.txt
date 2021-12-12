
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#find_package(LIBVLC REQUIRED)
find_package(LIBVLC REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS
    Multimedia
    MultimediaWidgets)

include_directories(
    ../
    ${LIBVLC_INCLUDE_DIR}
    ${Qt5Multimedia_INCLUDE_DIRS}
    ${Qt5MultimediaWidgets_INCLUDE_DIRS})

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  video_dialog.ui  )



add_library(PublisherVideoViewer SHARED
    video_viewer.cpp
    video_dialog.cpp
    ${UI_SRC}  )

target_link_libraries(PublisherVideoViewer
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    ${LIBVLC_LIBRARY}
    plotjuggler_base
    )

install(TARGETS PublisherVideoViewer DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

