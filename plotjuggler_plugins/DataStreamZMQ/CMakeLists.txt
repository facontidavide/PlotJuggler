# Check for vcpkg, conan, or manual build environments
if(BUILDING_WITH_VCPKG)
    message(STATUS "Finding ZeroMQ with vcpkg")
elseif(BUILDING_WITH_CONAN)
    message(STATUS "Finding ZeroMQ with conan")
else()
    message(STATUS "Finding ZeroMQ without package managers")

    # Find ZeroMQ using PkgConfig
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZMQ REQUIRED libzmq)

    # Set the ZeroMQ libraries and include directories for manual configuration
    set(ZeroMQ_LIBRARIES ${ZMQ_LIBRARIES})
    set(ZeroMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIRS})
    set(ZeroMQ_LIBRARY_DIRS /opt/homebrew/lib)  # Add this line

    # Add the library path explicitly
    link_directories(${ZeroMQ_LIBRARY_DIRS})  # Add this line
endif()

# Find ZeroMQ library
find_package(ZeroMQ QUIET)

# Check if ZeroMQ was found
if(ZeroMQ_FOUND)
    message(STATUS "[ZeroMQ] found")

    # Add QT definitions if needed
    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    # Wrap the UI file for Qt
    QT5_WRAP_UI(UI_SRC datastream_zmq.ui)

    # Add the DataStreamZMQ library
    add_library(DataStreamZMQ SHARED datastream_zmq.cpp ${UI_SRC})

    # Link Qt5Widgets and the plotjuggler_base target to DataStreamZMQ
    target_link_libraries(DataStreamZMQ ${Qt5Widgets_LIBRARIES} plotjuggler_base)

    # Handle different library linking for vcpkg, conan, or manual builds
    if(BUILDING_WITH_VCPKG OR BUILDING_WITH_CONAN)
        target_link_libraries(DataStreamZMQ libzmq-static)
    else()
        target_include_directories(DataStreamZMQ PRIVATE ${ZeroMQ_INCLUDE_DIRS})
        target_link_libraries(DataStreamZMQ zmq)  # Changed this to just 'zmq'
    endif()

    # Install the plugin
    install(TARGETS DataStreamZMQ DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
else()
    message(STATUS "[ZeroMQ] not found. Skipping plugin DataStreamZMQ.")
endif()
