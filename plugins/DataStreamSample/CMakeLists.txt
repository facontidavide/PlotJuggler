
include_directories( ./ ../  ../../include  ../../common)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)


#QT5_WRAP_UI ( UI_SRC  ../common/selectlistdialog.ui  )


SET( SRC
    datastream_sample.cpp
    ../../include/PlotJuggler/datastreamer_base.h
    )

add_library(DataStreamSample SHARED ${SRC} ${UI_SRC}  )


target_link_libraries(DataStreamSample  ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES})

if(COMPILING_WITH_CATKIN)
    install(TARGETS DataStreamSample
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
else()
    install(TARGETS DataStreamSample DESTINATION bin  )
endif()
