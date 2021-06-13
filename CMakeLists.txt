cmake_minimum_required(VERSION 3.10)

PROJECT(plotjuggler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}  "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_definitions(
    -DPJ_MAJOR_VERSION="3"
    -DPJ_MINOR_VERSION="2"
    -DPJ_PATCH_VERSION="0" )

include_directories( plotjuggler_base/include )

# http://answers.ros.org/question/230877/optionally-build-a-package-with-catkin/
if( CATKIN_DEVEL_PREFIX OR catkin_FOUND OR CATKIN_BUILD_BINARY_PACKAGE)
    set(COMPILING_WITH_CATKIN 1)

    message(STATUS "--------------------------------------------------")
    message(STATUS "PlotJuggler is being built using CATKIN.")
    message(STATUS "--------------------------------------------------")

    find_package(catkin REQUIRED COMPONENTS roslib )

    catkin_package(
        INCLUDE_DIRS  plotjuggler_base/include
        LIBRARIES  plotjuggler_plugin_base
        CATKIN_DEPENDS roslib
    )

message(${catkin_INCLUDE_DIRS})

    include_directories(${catkin_INCLUDE_DIRS})
    add_definitions(-DCOMPILED_WITH_CATKIN)

elseif( DEFINED ENV{AMENT_PREFIX_PATH})
    set(COMPILING_WITH_AMENT 1)
    
    find_package(ament_index_cpp REQUIRED)
    include_directories(${ament_index_cpp_INCLUDE_DIRS})

    message(STATUS "--------------------------------------------------")
    message(STATUS "PlotJuggler is being built using AMENT.")
    message(STATUS "--------------------------------------------------")

    add_definitions(-DCOMPILED_WITH_AMENT)

else()
    message(STATUS "--------------------------------------------------")
    message(STATUS "PlotJuggler is being WITHOUT any ROS support")
    message(STATUS "--------------------------------------------------")
endif()

cmake_policy (SET CMP0020 NEW)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)


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
    PrintSupport
    Concurrent
    Xml
    Svg
    OpenGL
    WebSockets
    Network)

set( QT_LINK_LIBRARIES
    Qt5::Core
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::Xml
    Qt5::Concurrent
    Qt5::Svg
    Qt5::OpenGL
    Qt5::WebSockets
    )


if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

IF (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC")
else()
    set(CMAKE_WIN32_EXECUTABLE ON)
    set(GUI_TYPE WIN32)
ENDIF()

find_package(ZMQ)

include_directories(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Concurrent_INCLUDE_DIRS}
    ${Qt5PrintSupport_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${Qt5WebSockets_INCLUDE_DIRS}
    ${Qt5Svg_INCLUDE_DIRS}
    ${Qt5OpenGL_INCLUDE_DIRS}
    ./3rdparty/
    ./3rdparty/color_widgets/include
    ./3rdparty/Qt-Advanced-Docking/include
    ./3rdparty/paho.mqtt.c/src
    ./3rdparty/qwt/src
    ./3rdparty/sol
    ./3rdparty/lua-5.4.1/src
    ./3rdparty/liblsl/src
    ./3rdparty/liblsl/include
)


#########################  INSTALL  ####################################

if(COMPILING_WITH_CATKIN)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_LIB_DESTINATION})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_BIN_DESTINATION})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_BIN_DESTINATION})

elseif(COMPILING_WITH_AMENT)
    find_package(ament_cmake REQUIRED)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    ament_package()

else()
    set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

set(DESKTOP_ICON_DIR "${CMAKE_INSTALL_PREFIX}/share/icons")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/PlotJuggler.desktop
        DESTINATION  "${CMAKE_INSTALL_PREFIX}/share/applications"  )

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/plotjuggler.svg
        DESTINATION ${DESKTOP_ICON_DIR}  )

######################## Main App, Libraries amd Plugins ###################################

if (NOT WIN32)
  add_subdirectory( 3rdparty/backward-cpp )
endif()

add_subdirectory( 3rdparty/color_widgets )
add_subdirectory( 3rdparty/Qt-Advanced-Docking )
add_subdirectory( 3rdparty/qwt/src )

add_subdirectory( 3rdparty/lua-5.4.1 )

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message("git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/3rdparty/paho.mqtt.c/CMakeLists.txt")
    SET(PAHO_ENABLE_TESTING FALSE CACHE BOOL "Build tests and run" FORCE)
    SET(PAHO_ENABLE_CPACK FALSE CACHE BOOL "Enable CPack" FORCE)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        SET(PAHO_BUILD_STATIC FALSE CACHE BOOL "Build static library" FORCE)
        SET(PAHO_BUILD_SHARED TRUE CACHE BOOL "Build shared library" FORCE)
    else()
        SET(PAHO_BUILD_STATIC TRUE CACHE BOOL "Build static library" FORCE)
        SET(PAHO_BUILD_SHARED FALSE CACHE BOOL "Build shared library" FORCE)
    endif()

    SET(CMAKE_INSTALL_INCLUDEDIR )

    add_subdirectory( 3rdparty/paho.mqtt.c EXCLUDE_FROM_ALL)
    add_subdirectory( plotjuggler_plugins/DataStreamMQTT )
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/3rdparty/liblsl/CMakeLists.txt" AND NOT WIN32)
    SET(LSL_ENABLE_TESTING FALSE CACHE BOOL "Build tests and run" FORCE)
    SET(LSL_ENABLE_CPACK FALSE CACHE BOOL "Enable CPack" FORCE)
    SET(LSL_BUILD_STATIC TRUE CACHE BOOL "Build static library" FORCE)
    SET(LSL_BUILD_SHARED FALSE CACHE BOOL "Build shared library" FORCE)
    add_subdirectory( 3rdparty/liblsl )
    add_subdirectory( plotjuggler_plugins/DataStreamLSL )
endif()

add_subdirectory( plotjuggler_base )
add_subdirectory( plotjuggler_app )

add_subdirectory( plotjuggler_plugins/DataLoadCSV )
add_subdirectory( plotjuggler_plugins/DataLoadULog )
add_subdirectory( plotjuggler_plugins/DataStreamSample )
add_subdirectory( plotjuggler_plugins/DataStreamWebsocket )
add_subdirectory( plotjuggler_plugins/DataStreamUDP )
add_subdirectory( plotjuggler_plugins/StatePublisherCSV )


if( ZMQ_FOUND )
  add_subdirectory( plotjuggler_plugins/DataStreamZMQ )
endif()

