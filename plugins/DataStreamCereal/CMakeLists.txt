
include_directories(../)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  datastream_cereal.ui  )

SET( SRC datastream_cereal.cpp )

add_library(DataStreamCereal SHARED ${SRC} ${UI_SRC}  )

target_link_libraries(DataStreamCereal
    ${Qt5Widgets_LIBRARIES}
    plotjuggler_plugin_base
    zmq
    )


if(COMPILING_WITH_CATKIN)
    install(TARGETS DataStreamCereal
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
elseif(COMPILING_WITH_AMENT)
    install(TARGETS DataStreamCereal
            DESTINATION  lib/${PROJECT_NAME} )
else()
    install(TARGETS DataStreamCereal DESTINATION bin  )
endif()


