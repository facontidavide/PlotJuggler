project(plotjuggler)
cmake_minimum_required(VERSION 2.8)

# http://answers.ros.org/question/230877/optionally-build-a-package-with-catkin/
if( CATKIN_DEVEL_PREFIX OR catkin_FOUND OR CATKIN_BUILD_BINARY_PACKAGE)
    set(COMPILING_WITH_CATKIN 1)
endif()

SET(PLOTJUGGLER_VERSION_MAJOR 1)
SET(PLOTJUGGLER_VERSION_MINOR 8)
SET(PLOTJUGGLER_VERSION_PATCH 3)

add_definitions(
    -DPJ_MAJOR_VERSION=${PLOTJUGGLER_VERSION_MAJOR}
    -DPJ_MINOR_VERSION=${PLOTJUGGLER_VERSION_MINOR}
    -DPJ_PATCH_VERSION=${PLOTJUGGLER_VERSION_PATCH} )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -fno-omit-frame-pointer")

find_package(Qt5 REQUIRED COMPONENTS Core Widgets PrintSupport Concurrent Xml Qml)
find_package(Qt5 QUIET COMPONENTS Svg)
find_package(Qt5 QUIET COMPONENTS WebSockets)

set( QT_LINK_LIBRARIES
    Qt5::Core
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::Xml
    Qt5::Concurrent
    Qt5::Qml
    )

if( NOT Qt5Svg_FOUND)
    message(STATUS "Can't find Qt5Svg in your system. Have you tried [sudo apt-get install libqt5svg5-dev] ?")
    add_definitions( -DQWT_NO_SVG )
else()
    set( QT_LINK_LIBRARIES ${QT_LINK_LIBRARIES} Qt5::Svg)
endif()

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

if(COMPILING_WITH_CATKIN)
    message(STATUS "---------------------------------------------------------------------")
    message(STATUS "PlotJuggler is being built using CATKIN. ROS plugins will be compiled")
    message(STATUS "---------------------------------------------------------------------")

    find_package(catkin REQUIRED COMPONENTS
        rosbag
        rosbag_storage
        roscpp
        roscpp_serialization
        ros_type_introspection
        rostime
        topic_tools
        )
    catkin_package(
        INCLUDE_DIRS include
        CATKIN_DEPENDS ros_type_introspection
        DEPENDS
        )

    INCLUDE_DIRECTORIES( ${catkin_INCLUDE_DIRS} )
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
    ${Qt5Svg_INCLUDE_DIRS}
    ${Qt5WebSockets_INCLUDE_DIRS}
    )

include_directories( ./qwt/src
    ./include
    ./color_widgets/include )

if(COMPILING_WITH_CATKIN)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_LIB_DESTINATION})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_BIN_DESTINATION})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_BIN_DESTINATION})
    
    install(DIRECTORY site/       DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/docs )
    install(DIRECTORY docs/images DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/docs )
    add_definitions( -DPJ_DOCUMENTATION_DIR="${CMAKE_INSTALL_PREFIX}/${CATKIN_PACKAGE_SHARE_DESTINATION}/docs" )
else()
    set(CMAKE_INSTALL_PREFIX "/usr/local")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    install(DIRECTORY site/       DESTINATION ${CMAKE_BINARY_DIR}/share/docs )
    install(DIRECTORY docs/images DESTINATION ${CMAKE_BINARY_DIR}/share/docs )
    add_definitions( -DPJ_DOCUMENTATION_DIR="${CMAKE_INSTALL_PREFIX}/share/docs" )
endif()


add_subdirectory(backward-cpp)

add_subdirectory( color_widgets )
add_subdirectory( qwt/src )
add_subdirectory( plotter_gui )

add_subdirectory( plugins/DataLoadCSV )
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


if(COMPILING_WITH_CATKIN)
    add_subdirectory( plugins/ROS )
endif()


#########################  create a package ####################################



SET(CPACK_PACKAGE_CONTACT "davide.faconti@gmail.com")

SET(CPACK_PACKAGE_NAME "plotjuggler${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "PlotJuggler: juggle with data")
SET(CPACK_PACKAGE_VERSION_MAJOR ${PLOTJUGGLER_VERSION_MAJOR} )
SET(CPACK_PACKAGE_VERSION_MINOR ${PLOTJUGGLER_VERSION_MINOR} )
SET(CPACK_PACKAGE_VERSION_PATCH ${PLOTJUGGLER_VERSION_PATCH} )
SET(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
SET(CPACK_PACKAGE_VENDOR "Icarus Technology")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "PlotJuggler")

INCLUDE(CPack)

IF (NOT WIN32)
    SET(CPACK_GENERATOR "TGZ;DEB")
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Davide Faconti")
    SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local/")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "qt5-default, libqt5svg5-dev")

else()
    include(CPackIFW)
    get_target_property(QtCore_LOCATION Qt5::Core LOCATION)
    get_filename_component(QT_DLL_DIR ${QtCore_LOCATION} PATH)
    install(FILES
      ${QT_DLL_DIR}/Qt5Core.dll
      ${QT_DLL_DIR}/Qt5Gui.dll
      ${QT_DLL_DIR}/Qt5Widgets.dll
      ${QT_DLL_DIR}/Qt5Xml.dll
      ${QT_DLL_DIR}/Qt5Svg.dll
      ${QT_DLL_DIR}/libgcc_s_dw2-1.dll
      ${QT_DLL_DIR}/libwinpthread-1.dll
      ${QT_DLL_DIR}/libstdc++-6.dll
      DESTINATION ./bin
    )
    install(FILES
      ${QT_DLL_DIR}/../plugins/platforms/qwindows.dll
      DESTINATION plugins/platforms
    )

    CPACK_ADD_COMPONENT(PlotJuggler
        DISPLAY_NAME "PlotJuggler"
        DESCRIPTION  "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}" )

    CPACK_IFW_CONFIGURE_COMPONENT(PlotJuggler
        VERSION "${CPACK_PACKAGE_VERSION}"
        LICENSES "LGPL" "${CMAKE_SOURCE_DIR}/LICENSE")
endif()



