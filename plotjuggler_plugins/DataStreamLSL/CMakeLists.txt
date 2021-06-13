include_directories(../)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  datastream_lsl.ui  )

SET( SRC datastream_lsl.cpp streamer.cpp )

add_library(DataStreamLSL SHARED ${SRC} ${UI_SRC}  )

target_link_libraries(DataStreamLSL
    ${Qt5Widgets_LIBRARIES}
    plotjuggler_plugin_base
    lsl-static
    lslboost
    )


if(COMPILING_WITH_CATKIN)
    install(TARGETS DataStreamLSL
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
elseif(COMPILING_WITH_AMENT)
    install(TARGETS DataStreamLSL
            DESTINATION  lib/${PROJECT_NAME} )
else()
    install(TARGETS DataStreamLSL DESTINATION bin  )
endif()


