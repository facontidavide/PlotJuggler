# Install script for directory: /home/alvaro/Proyects/PlotJuggler/plotjuggler_plugins

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/tmp")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataLoadCSV/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataLoadMCAP/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataLoadParquet/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataLoadULog/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataStreamSample/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataStreamWebsocket/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataStreamUDP/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataStreamMQTT/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataStreamZMQ/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/StatePublisherCSV/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/VideoViewer/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/ToolboxQuaternion/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/ToolboxFFT/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/ToolboxLuaEditor/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/ParserProtobuf/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/ParserROS/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/ParserDataTamer/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/ParserLineInflux/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/PluginsZcm/cmake_install.cmake")
endif()

