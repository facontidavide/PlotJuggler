if(BUILDING_WITH_VCPKG)
    message(STATUS "Finding ZeroMQ with vcpkg")
    set(ZeroMQ_LIBRARIES libzmq libzmq-static)
elseif(BUILDING_WITH_CONAN)
    message(STATUS "Finding ZeroMQ with conan")
    set(ZeroMQ_LIBRARIES libzmq-static)
else()
    message(STATUS "Finding ZeroMQ without package managers")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
endif()

find_package(ZeroMQ QUIET)

if(ZeroMQ_FOUND)

    message(STATUS "[ZeroMQ] found")

    include_directories(../ ${ZeroMQ_INCLUDE_DIR})

    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    QT5_WRAP_UI ( UI_SRC  datastream_zmq.ui  )

    SET( SRC datastream_zmq.cpp )

    add_library(DataStreamZMQ SHARED ${SRC} ${UI_SRC}  )

    target_link_libraries(DataStreamZMQ
        ${Qt5Widgets_LIBRARIES}
        plotjuggler_base
        ${ZeroMQ_LIBRARIES}
        )

    install(TARGETS DataStreamZMQ DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
else()
    message("[ZeroMQ] not found. Skipping plugin DataStreamZMQ.")
endif()

