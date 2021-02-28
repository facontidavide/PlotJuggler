
include_directories(  ../ )

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  ../selectlistdialog.ui  )

SET( SRC
    dataload_can.cpp
    ../selectlistdialog.h
    )

add_library(DataLoadCAN SHARED ${SRC} ${UI_SRC}  )
target_link_libraries(DataLoadCAN
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    dbcppp
    plotjuggler_plugin_base)

if(COMPILING_WITH_CATKIN)
    install(TARGETS DataLoadCAN
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
elseif(COMPILING_WITH_AMENT)
    install(TARGETS DataLoadCAN
            DESTINATION  lib/${PROJECT_NAME} )
else()
    install(TARGETS DataLoadCAN DESTINATION bin  )
endif()
