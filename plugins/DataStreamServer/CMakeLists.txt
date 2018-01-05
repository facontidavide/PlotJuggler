
include_directories( ./ ../  ../../include  ../../common ${Qt5WebSockets_INCLUDE_DIRS})

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_WEBSOCKETS)

find_package(Qt5 REQUIRED COMPONENTS websockets )

#QT5_WRAP_UI ( UI_SRC  ../common/selectlistdialog.ui  )


SET( SRC
    DataStreamServer.cpp
    ../../include/PlotJuggler/datastreamer_base.h
    )

add_library(DataStreamServer SHARED ${SRC} ${UI_SRC}  )


target_link_libraries(DataStreamServer  ${Qt5Widgets_LIBRARIES} Qt5::WebSockets Qt5::Xml)

if(COMPILING_WITH_CATKIN)
    install(TARGETS DataStreamServer
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
else()
    install(TARGETS DataStreamServer DESTINATION bin  )
endif()
