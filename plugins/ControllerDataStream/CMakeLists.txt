include_directories( ./ ../  ../../include  ../../common)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)


QT5_WRAP_UI ( UI_SRC  )

SET( SRC
        ControllerStreamUARTDecoder.cpp
        ControllerDataStream.cpp
        ../../include/PlotJuggler/datastreamer_base.h
        )


add_library(ControllerStream SHARED ${SRC} ${UI_SRC} )
target_link_libraries(ControllerStream  ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES})
target_compile_definitions(ControllerStream PRIVATE PLUGIN_NAME="Zoov Controller Stream")

if(COMPILING_WITH_CATKIN)
    install(TARGETS ControllerStream
            ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
            LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
            RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
else()
    install(TARGETS ControllerStream DESTINATION bin  )

endif()