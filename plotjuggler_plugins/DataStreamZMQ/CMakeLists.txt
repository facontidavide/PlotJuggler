include(FetchContent)
option(BUILD_TESTS "" OFF)
option(WITH_LIBSODIUM "" OFF)
option(BUILD_STATIC "" ON)
FetchContent_Declare(
        libzmq
        URL https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz
        URL_HASH MD5=c897d4005a3f0b8276b00b7921412379
        )
FetchContent_MakeAvailable(libzmq)
set(ZeroMQ_LIBRARY ${ZEROMQ_LIBRARY}-static)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  datastream_zmq.ui  )

SET( SRC datastream_zmq.cpp )

add_library(DataStreamZMQ SHARED ${SRC} ${UI_SRC}  )

target_link_libraries(DataStreamZMQ
    ${Qt5Widgets_LIBRARIES}
    plotjuggler_base
    ${ZeroMQ_LIBRARY}
    )

install(TARGETS DataStreamZMQ DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

