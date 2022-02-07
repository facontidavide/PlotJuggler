cmake_minimum_required(VERSION 3.7)


find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
find_library(MOSQUITTO_LIBRARY NAMES libmosquitto mosquitto)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MOSQUITTO DEFAULT_MSG MOSQUITTO_LIBRARY MOSQUITTO_INCLUDE_DIR)

if(MOSQUITTO_FOUND)

    set(MOSQUITTO_LIBRARIES ${MOSQUITTO_LIBRARY})

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
        ${MOSQUITTO_LIBRARIES}
        Qt5::Network
        plotjuggler_base
        )
    target_include_directories(DataStreamMQTT_Mosquitto
        PUBLIC ${MOSQUITTO_INCLUDE_DIR})

    install(TARGETS DataStreamMQTT_Mosquitto
        DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

else()
    message("[libosquittopp-dev] not found. Skipping plugin DataStreamMQTT.")
endif()


