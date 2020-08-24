# Qt related stuff
set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)

find_package(Qt5 COMPONENTS WebSockets REQUIRED)


QT5_WRAP_UI ( COMMON_UI_SRC
    qwsdialog.ui
    dialog_select_ros_topics.ui
    )

SET( COMMON_SRC
    dialog_select_ros_topics.h
    dialog_select_ros_topics.cpp
   )

set(PLUGIN_BASE_DIR ${PROJECT_SOURCE_DIR}/include/PlotJuggler)

include_directories(
    ./ ../
    ${PROJECT_SOURCE_DIR}/include/
)

list( APPEND COMMON_SRC
    qwsdialog.cpp
    )

add_library( commonLibs STATIC ${COMMON_SRC} ${COMMON_UI_SRC} )
target_compile_definitions(commonLibs PUBLIC DISABLE_RULE_EDITING)
target_link_libraries( commonLibs PRIVATE
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    ${Qt5WebSockets_LIBRARIES}
    fmt::fmt-header-only
)

#########
add_library(DataStreamROSWS SHARED
        DataStreamROSWS/datastream_ROSWS.cpp
        ${PLUGIN_BASE_DIR}/datastreamer_base.h )
target_link_libraries(DataStreamROSWS commonLibs )

install(TARGETS
        DataStreamROSWS
    DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
    )
