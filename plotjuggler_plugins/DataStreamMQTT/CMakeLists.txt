cmake_minimum_required(VERSION 3.10.2)

find_package(mosquitto QUIET)

if(mosquitto_LIBRARIES)

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
    find_package(mosquitto QUIET)
else()
    set(mosquitto_FOUND True)
endif()

if(mosquitto_FOUND)

    message(STATUS "[MOSQUITTO] found.")
    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    QT5_WRAP_UI ( UI_SRC  datastream_mqtt.ui  )

    SET( SRC
        datastream_mqtt.h
        datastream_mqtt.cpp
        mqtt_dialog.h
        mqtt_dialog.cpp
        mqtt_client.h
        mqtt_client.cpp
        )

    add_library(DataStreamMQTT_Mosquitto SHARED ${SRC} ${UI_SRC}  )

    target_link_libraries(DataStreamMQTT_Mosquitto
        ${Qt5Widgets_LIBRARIES}
        Qt5::Network
        Qt5::Svg
        ${MOSQUITTO_LIBRARIES}
        plotjuggler_base
        )
    target_include_directories(DataStreamMQTT_Mosquitto
        PUBLIC ${MOSQUITTO_INCLUDE_DIR})

    install(TARGETS DataStreamMQTT_Mosquitto
        DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

else()
    message("[MOSQUITTO] not found. Skipping plugin DataStreamMQTT.")
endif()


