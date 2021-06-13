
include_directories(
    ../../include
    paho.mqtt.c/src)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  datastream_mqtt.ui  )

SET( SRC datastream_mqtt.cpp )

add_library(DataStreamMQTT SHARED ${SRC} ${UI_SRC}  )

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(DataStreamMQTT
        ${Qt5Widgets_LIBRARIES} plotjuggler_plugin_base
        paho-mqtt3c
        paho-mqtt3a  )
else()
    target_link_libraries(DataStreamMQTT
        ${Qt5Widgets_LIBRARIES} plotjuggler_plugin_base
        paho-mqtt3c-static
        paho-mqtt3a-static)
endif()


if(COMPILING_WITH_CATKIN)
    install(TARGETS DataStreamMQTT
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
elseif(COMPILING_WITH_AMENT)
    install(TARGETS DataStreamMQTT
            DESTINATION  lib/${PROJECT_NAME} )
else()
    install(TARGETS DataStreamMQTT DESTINATION bin  )
endif()




