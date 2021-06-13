
include_directories(  ../ )

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC
    ../selectlistdialog.ui
    ulog_parameters_dialog.ui
    )

SET( SRC
    dataload_ulog.cpp
    ulog_parser.cpp
    ulog_parameters_dialog.cpp
    )

add_library(DataLoadULog SHARED ${SRC} ${UI_SRC}  )

target_link_libraries(DataLoadULog
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    plotjuggler_plugin_base)

if(COMPILING_WITH_CATKIN)
    install(TARGETS DataLoadULog
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
elseif(COMPILING_WITH_AMENT)
    install(TARGETS DataLoadULog
            DESTINATION  lib/${PROJECT_NAME} )
else()
    install(TARGETS DataLoadULog DESTINATION bin  )
endif()
