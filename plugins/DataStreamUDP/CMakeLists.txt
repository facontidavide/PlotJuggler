
include_directories( ../ )

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  udp_server.ui  )

SET( SRC udp_server.cpp )

add_library(DataStreamUDP SHARED ${SRC} ${UI_SRC}  )

target_link_libraries(DataStreamUDP
    ${Qt5Widgets_LIBRARIES}
    Qt5::Network
    plotjuggler_plugin_base
    )


if(COMPILING_WITH_CATKIN)
    install(TARGETS DataStreamUDP
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
elseif(COMPILING_WITH_AMENT)
    install(TARGETS DataStreamUDP
            DESTINATION  lib/${PROJECT_NAME} )
else()
    install(TARGETS DataStreamUDP DESTINATION bin  )
endif()


