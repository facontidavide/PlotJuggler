if(BUILDING_WITH_VCPKG)
    message(STATUS "Finding ZeroMQ with vcpkg")
elseif(BUILDING_WITH_CONAN)
    message(STATUS "Finding ZeroMQ with conan")
else()
    message(STATUS "Finding ZeroMQ without package managers")
endif()

find_package(ZeroMQ QUIET)

if(ZeroMQ_FOUND)
    message(STATUS "[ZeroMQ] found at ${ZeroMQ_INCLUDE_DIR} and ${ZeroMQ_LIBRARY}")

    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)
    include_directories(${ZeroMQ_INCLUDE_DIR})

    QT5_WRAP_UI(UI_SRC datastream_zmq.ui)

    add_library(DataStreamZMQ SHARED datastream_zmq.cpp ${UI_SRC})

    target_include_directories(DataStreamZMQ PRIVATE ${ZeroMQ_INCLUDE_DIRS})

    target_link_libraries(DataStreamZMQ
        ${Qt5Widgets_LIBRARIES}
        plotjuggler_base
        ${ZeroMQ_LIBRARIES}
    )

    if(BUILDING_WITH_VCPKG OR BUILDING_WITH_CONAN)
        target_link_libraries(DataStreamZMQ libzmq-static)
    endif()

    install(TARGETS DataStreamZMQ
        DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}
    )
else()
    message(WARNING "[ZeroMQ] not found. Skipping plugin DataStreamZMQ.")
endif()
