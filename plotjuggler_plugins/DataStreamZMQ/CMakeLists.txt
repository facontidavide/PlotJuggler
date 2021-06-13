
include_directories(../)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  datastream_zmq.ui  )

SET( SRC datastream_zmq.cpp )

add_library(DataStreamZMQ SHARED ${SRC} ${UI_SRC}  )

target_link_libraries(DataStreamZMQ
    ${Qt5Widgets_LIBRARIES}
    plotjuggler_plugin_base
    zmq
    )


if(COMPILING_WITH_CATKIN)
    install(TARGETS DataStreamZMQ
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
elseif(COMPILING_WITH_AMENT)
    install(TARGETS DataStreamZMQ
            DESTINATION  lib/${PROJECT_NAME} )
else()
    install(TARGETS DataStreamZMQ DESTINATION bin  )
endif()


