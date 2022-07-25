
set(CMAKE_CONFIG_PATH ${CMAKE_MODULE_PATH}  "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CONFIG_PATH}")

find_package(ZMQ QUIET)

if(ZMQ_FOUND)

    message(STATUS "ZMQ_FOUND")

    include_directories(../ ${ZMQ_INCLUDE_DIR})

    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    QT5_WRAP_UI ( UI_SRC  datastream_zmq.ui  )

    SET( SRC datastream_zmq.cpp )

    add_library(DataStreamZMQ SHARED ${SRC} ${UI_SRC}  )

    target_link_libraries(DataStreamZMQ
        ${Qt5Widgets_LIBRARIES}
        plotjuggler_base
        ${ZMQ_LIBRARY}
        )

    install(TARGETS DataStreamZMQ DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
else()
    message("[libzmq-dev] not found. Skipping plugin DataStreamZMQ.")
endif()

