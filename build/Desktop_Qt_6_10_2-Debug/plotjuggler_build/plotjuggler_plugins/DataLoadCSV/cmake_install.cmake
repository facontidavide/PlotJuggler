# Install script for directory: /home/alvaro/Proyects/PlotJuggler/plotjuggler_plugins/DataLoadCSV

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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/tmp/lib/plotjuggler/plugins/libDataLoadCSV.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/tmp/lib/plotjuggler/plugins/libDataLoadCSV.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/tmp/lib/plotjuggler/plugins/libDataLoadCSV.so"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/tmp/lib/plotjuggler/plugins/libDataLoadCSV.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/tmp/lib/plotjuggler/plugins" TYPE SHARED_LIBRARY FILES "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/bin/libDataLoadCSV.so")
  if(EXISTS "$ENV{DESTDIR}/tmp/lib/plotjuggler/plugins/libDataLoadCSV.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/tmp/lib/plotjuggler/plugins/libDataLoadCSV.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/bin/strip" "$ENV{DESTDIR}/tmp/lib/plotjuggler/plugins/libDataLoadCSV.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/plotjuggler_build/plotjuggler_plugins/DataLoadCSV/CMakeFiles/DataLoadCSV.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

