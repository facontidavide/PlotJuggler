if(BUILDING_WITH_VCPKG)
    message(STATUS "Finding ZeroMQ with vcpkg")
elseif(BUILDING_WITH_CONAN)
    message(STATUS "Finding ZeroMQ with conan")
else()
    message(STATUS "Finding ZeroMQ without package managers")
    set(ZeroMQ_LIBS ${ZeroMQ_LIBRARIES})
endif()

find_package(ZeroMQ QUIET)

if(ZeroMQ_FOUND)
    message(STATUS "[ZeroMQ] found")

    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    QT5_WRAP_UI ( UI_SRC  datastream_zmq.ui  )

    add_library(DataStreamZMQ SHARED datastream_zmq.cpp ${UI_SRC}  )

    target_link_libraries(DataStreamZMQ ${Qt5Widgets_LIBRARIES} plotjuggler_base)

    if(BUILDING_WITH_VCPKG OR BUILDING_WITH_CONAN)
        target_link_libraries(DataStreamZMQ libzmq-static)
    else()
        target_link_libraries(DataStreamZMQ ${ZeroMQ_LIBRARIES})
    endif()

    install(TARGETS DataStreamZMQ DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
else()
    message("[ZeroMQ] not found. Skipping plugin DataStreamZMQ.")
endif()
