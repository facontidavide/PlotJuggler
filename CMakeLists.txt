cmake_minimum_required(VERSION 3.5)

cmake_policy(SET CMP0048 OLD)
PROJECT(plotjuggler)

add_definitions(
    -DPJ_MAJOR_VERSION="2"
    -DPJ_MINOR_VERSION="8"
    -DPJ_PATCH_VERSION="3" )

# http://answers.ros.org/question/230877/optionally-build-a-package-with-catkin/
if( CATKIN_DEVEL_PREFIX OR catkin_FOUND OR CATKIN_BUILD_BINARY_PACKAGE)
    set(COMPILING_WITH_CATKIN 1)

    message(STATUS "---------------------------------------------------------------------")
    message(STATUS "PlotJuggler is being built using CATKIN. ROS plugins will be compiled")
    message(STATUS "---------------------------------------------------------------------")

    set(ROS_DEPENDENCIES
        rosbag_storage
        roscpp
        roscpp_serialization
        tf
        tf2_ros
        diagnostic_msgs
        nav_msgs
        sensor_msgs
        plotjuggler_msgs )

    find_package(catkin REQUIRED COMPONENTS ${ROS_DEPENDENCIES} )

    catkin_package(
        INCLUDE_DIRS
        CATKIN_DEPENDS ${ROS_DEPENDENCIES}  )

elseif( DEFINED ENV{AMENT_PREFIX_PATH})
    set(COMPILING_WITH_AMENT 1)
    
    message(STATUS "---------------------------------------------------------------------")
    message(STATUS "PlotJuggler is being built using AMENT. ROS2 plugins will be compiled")
    message(STATUS "---------------------------------------------------------------------")
else()
    message(STATUS "---------------------------------------------------------------------")
    message(STATUS "PlotJuggler is being WITHOUT any ROS support")
    message(STATUS "---------------------------------------------------------------------")
endif()

cmake_policy (SET CMP0020 NEW)

add_subdirectory( 3rdparty/fmt )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)

if(NOT CMAKE_VERSION VERSION_LESS 3.1)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fno-omit-frame-pointer")
endif()

if(APPLE AND EXISTS /usr/local/opt/qt5)
    # Homebrew installs Qt5 (up to at least 5.9.1) in
    # /usr/local/qt5, ensure it can be found by CMake since
    # it is not in the default /usr/local prefix.
    # source: https://github.com/Homebrew/homebrew-core/issues/8392#issuecomment-325226494
    list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
    set(CMAKE_MACOSX_RPATH 1)
endif()


find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Multimedia
    MultimediaWidgets
    PrintSupport
    Concurrent
    Qml
    Xml
    Svg)

find_package(Qt5 QUIET COMPONENTS WebSockets)

set( QT_LINK_LIBRARIES
    Qt5::Core
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::Xml
    Qt5::Concurrent
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    Qt5::Qml
    Qt5::Svg
    )

add_definitions( -DQWT_NO_SVG )

if( NOT Qt5WebSockets_FOUND)
    message(STATUS "Can't find Qt5WebSockets in your system. Have you tried [sudo apt-get install libqt5websockets5-dev] ?")
    add_definitions( -DQWT_NO_SVG )
else()
    set( QT_LINK_LIBRARIES ${QT_LINK_LIBRARIES} Qt5::WebSockets)
endif()

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

IF (NOT WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC")
ENDIF()


include_directories(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Concurrent_INCLUDE_DIRS}
    ${Qt5PrintSupport_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${Qt5WebSockets_INCLUDE_DIRS}
    ${Qt5Multimedia_INCLUDE_DIRS}
    ${Qt5MultimediaWidgets_INCLUDE_DIRS}
    ${Qt5Svg_INCLUDE_DIRS}
    ./include
    ./3rdparty/qwt/src
    ./3rdparty/color_widgets/include
    ./3rdparty/fmt/include
    ./3rdparty/marl/include
    ./3rdparty/sol
    ./3rdparty/lua-5.3.4/src
)

#########################  INSTALL ####################################

if(COMPILING_WITH_CATKIN)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_LIB_DESTINATION})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_BIN_DESTINATION})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_BIN_DESTINATION})

elseif(COMPILING_WITH_AMENT)
    find_package(ament_cmake REQUIRED)

else()
    set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

#############################################################

if (NOT WIN32)
  add_subdirectory( 3rdparty/backward-cpp )
endif()

add_subdirectory( 3rdparty/color_widgets )
#add_subdirectory( 3rdparty/marl )
add_subdirectory( 3rdparty/qwt/src )
add_subdirectory( 3rdparty/lua-5.3.4 )
add_subdirectory( plotter_gui )

add_subdirectory( plugins/DataLoadCSV )
add_subdirectory( plugins/DataLoadULog )
add_subdirectory( plugins/DataStreamSample )

if (Qt5Widgets_VERSION VERSION_LESS 5.3.0)
    message(STATUS "Minimum Qt5 version for DataStreamWebSocket plugin is 5.3. Skipping plugins/DataStreamWebSocket")
else()
    if( NOT Qt5WebSockets_FOUND)
        message(STATUS "Qt5WebSockets not found. Skipping plugins/DataStreamWebSocket")
    else()
        add_subdirectory( plugins/DataStreamWebSocket )
    endif()
endif()

add_subdirectory( plugins/ROS )

if(COMPILING_WITH_AMENT)
    ament_package()
endif()

#############################################################

set(DESKTOP_ICON_DIR "${CMAKE_INSTALL_PREFIX}/share/icons")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/PlotJuggler.desktop
        DESTINATION  "${CMAKE_INSTALL_PREFIX}/share/applications"  )

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/plotjuggler.svg
        DESTINATION ${DESKTOP_ICON_DIR}  )

