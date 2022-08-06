# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

###############################################################################
# CMake build rules for FastCDR                                               #
###############################################################################
cmake_minimum_required(VERSION 2.8.12)

set(IS_TOP_LEVEL TRUE)
if(PROJECT_SOURCE_DIR)
    set(IS_TOP_LEVEL FALSE)
endif()

# Set CMAKE_BUILD_TYPE to Release by default.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

###############################################################################
# Version information                                                         #
###############################################################################

#If not set, copy over product version from configure.ac
file(STRINGS
    configure.ac
    LIB_MAJOR_VERSION_TMP
    REGEX "^m4_define\\(\\[?version_major\\]?, *\\[?[0-9]+\\]?\\)"
    )
file(STRINGS
    configure.ac
    LIB_MINOR_VERSION_TMP
    REGEX "^m4_define\\(\\[?version_minor\\]?, *\\[?[0-9]+\\]?\\)"
    )
file(STRINGS
    configure.ac
    LIB_PATCH_VERSION_TMP
    REGEX "^m4_define\\(\\[?version_micro\\]?, *\\[?[0-9]+\\]?\\)"
    )

string(REGEX REPLACE "m4_define\\(\\[?version_major\\]?, *\\[?([0-9]+)\\]?\\)"
    "\\1"
    LIB_MAJOR_VERSION
    ${LIB_MAJOR_VERSION_TMP}
    )
string(REGEX REPLACE "m4_define\\(\\[?version_minor\\]?, *\\[?([0-9]+)\\]?\\)"
    "\\1"
    LIB_MINOR_VERSION
    ${LIB_MINOR_VERSION_TMP}
    )
string(REGEX REPLACE "m4_define\\(\\[?version_micro\\]?, *\\[?([0-9]+)\\]?\\)"
    "\\1"
    LIB_PATCH_VERSION
    ${LIB_PATCH_VERSION_TMP}
    )

set(LIB_VERSION_STR "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_PATCH_VERSION}")

###############################################################################
# Project                                                                     #
###############################################################################
if(CMAKE_VERSION VERSION_LESS 3.0)
    project(fastcdr C CXX)
    set(PROJECT_VERSION_MAJOR "${LIB_MAJOR_VERSION}")
    set(PROJECT_VERSION_MINOR "${LIB_MINOR_VERSION}")
    set(PROJECT_VERSION_PATCH "${LIB_PATCH_VERSION}")
    set(PROJECT_VERSION "${LIB_VERSION_STR}")
else()
    cmake_policy(SET CMP0048 NEW)
    project(fastcdr VERSION "${LIB_VERSION_STR}" LANGUAGES C CXX)
endif()

set(PROJECT_NAME_STYLED "FastCDR")
set(PROJECT_NAME_LARGE "Fast CDR")
string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
set(${PROJECT_NAME}_DESCRIPTION_SUMMARY "C++ library for serialize using CDR serialization")
set(${PROJECT_NAME}_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} library provides two serialization mechanisms. One is the standard CDR serialization mechanism, while the other is a faster implementation that modifies the standard.")

message(STATUS "Configuring ${PROJECT_NAME_LARGE}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "To change the version modify the file configure.ac")

###############################################################################
# Generation of windows installers.
###############################################################################
option(EPROSIMA_INSTALLER "Activate the creation of a build to create windows installers" OFF)

###############################################################################
# GCC colors if using CCache
###############################################################################
if("${CMAKE_CXX_COMPILER_LAUNCHER}" STREQUAL "ccache" AND
        CMAKE_COMPILER_IS_GNUCXX AND
        CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4,8)
    add_compile_options(-fdiagnostics-color=always)
endif()

###############################################################################
# Default shared libraries
###############################################################################
# Global flag to cause add_library() to create shared libraries if on.
# If set to true, this will cause all libraries to be built shared
# unless the library was explicitly added as a static library.
option(BUILD_SHARED_LIBS "Create shared libraries by default" ON)

###############################################################################
# Load external eProsima projects.
###############################################################################

option(EPROSIMA_BUILD "Activate internal building" OFF)

if(EPROSIMA_INSTALLER)
    set(EPROSIMA_BUILD ON)
endif()

###############################################################################
# Test system configuration
###############################################################################
include(${PROJECT_SOURCE_DIR}/cmake/common/check_configuration.cmake)
check_stdcxx()
check_compile_feature()
check_endianness()
check_type_sizes()

###############################################################################
# Check MSVC architecture
###############################################################################
if(MSVC OR MSVC_IDE)
    check_msvc_arch()
endif()

###############################################################################
# Installation paths
###############################################################################
option(APPEND_PROJECT_NAME_TO_INCLUDEDIR
  "When ON headers are installed to a path ending with a folder called \
  ${PROJECT_NAME}. This avoids include directory search order issues when \
  overriding this package from a merged catkin, ament, or colcon workspace."
  OFF)

set(BIN_INSTALL_DIR bin/ CACHE PATH "Installation directory for binaries")
set(_include_dir "include/")
if(APPEND_PROJECT_NAME_TO_INCLUDEDIR)
  string(APPEND _include_dir "${PROJECT_NAME}/")
endif()
set(INCLUDE_INSTALL_DIR "${_include_dir}" CACHE PATH "Installation directory for C++ headers")
unset(_include_dir)
set(LIB_INSTALL_DIR lib${LIB_SUFFIX}/ CACHE PATH "Installation directory for libraries")
set(DATA_INSTALL_DIR share/ CACHE PATH "Installation directory for data")
if(WIN32)
    set(DOC_DIR "doc")
else()
    set(DOC_DIR "${DATA_INSTALL_DIR}/doc")
endif()
set(DOC_INSTALL_DIR ${DOC_DIR} CACHE PATH "Installation directory for documentation")
if(WIN32 AND EPROSIMA_INSTALLER AND IS_TOP_LEVEL)
    set(LICENSE_INSTALL_DIR . CACHE PATH "Installation directory for licenses")
else()
    set(LICENSE_INSTALL_DIR ${DATA_INSTALL_DIR}/${PROJECT_NAME} CACHE PATH "Installation directory for licenses")
endif()

###############################################################################
# Compile library.
###############################################################################
add_subdirectory(src/cpp)

###############################################################################
# Testing options
###############################################################################
option(EPROSIMA_BUILD_TESTS "Activate the building and execution unit tests and integral tests" OFF)

if(EPROSIMA_BUILD AND NOT EPROSIMA_INSTALLER AND NOT EPROSIMA_INSTALLER_MINION)
    set(EPROSIMA_BUILD_TESTS ON)
endif()

###############################################################################
# Testing
###############################################################################
if(EPROSIMA_BUILD_TESTS AND IS_TOP_LEVEL AND NOT EPROSIMA_INSTALLER)
    enable_testing()
    include(CTest)

    add_subdirectory(test)
endif()

###############################################################################
# Documentation
###############################################################################
# Add an option to toggle the generation of the API documentation.
option(BUILD_DOCUMENTATION "Use doxygen to create product documentation" OFF)
option(CHECK_DOCUMENTATION "Use doxygen to check code documentation" OFF)

if(EPROSIMA_INSTALLER)
    set(BUILD_DOCUMENTATION ON)
    set(DOXYGEN_SHOULD_SKIP_THIS_PUBLIC "DOXYGEN_SHOULD_SKIP_THIS_PUBLIC")
endif()

if(CHECK_DOCUMENTATION)
    set(BUILD_DOCUMENTATION ON)
endif()

if(BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "doxygen is needed to build the documentation. Please install it correctly")
    endif()
    if(UNIX)
        find_program(DOXYFILE_MAKE make)
        if(DOXYFILE_MAKE)
            message(STATUS "Found Make: ${DOXYFILE_MAKE}")
        else()
            message(FATAL_ERROR "make is needed to build the documentation. Please install it correctly")
        endif()
    elseif(WIN32)
        set(DOXYFILE_MAKE make.bat)
    endif()

    if(NOT CHECK_DOCUMENTATION)
        find_program(WGET_EXE wget)
        if(WGET_EXE)
            message(STATUS "Found WGet: ${WGET_EXE}")
        else()
            message(FATAL_ERROR "wget is needed to build the documentation. Please install it correctly")
        endif()
        find_program(UNZIP_EXE unzip)
        if(UNZIP_EXE)
            message(STATUS "Found Unzip: ${UNZIP_EXE}")
        else()
            message(FATAL_ERROR "unzip is needed to build the documentation. Please install it correctly")
        endif()
    endif()

    # Target to create documentation directories
    add_custom_target(docdirs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/doc
        COMMENT "Creating documentation directory" VERBATIM)

    ### Doxygen ########################3
    if(CHECK_DOCUMENTATION)
        set(USE_DOT NO)
    else()
        set(USE_DOT YES)
    endif()
    # Configure the template doxyfile for or specific project
    configure_file(doxyfile.in ${PROJECT_BINARY_DIR}/doxyfile @ONLY IMMEDIATE)
    # Add custom target to run doxygen when ever the project is build
    add_custom_target(doxygen
        COMMAND "${DOXYGEN_EXECUTABLE}" "${PROJECT_BINARY_DIR}/doxyfile"
        SOURCES "${PROJECT_BINARY_DIR}/doxyfile"
        COMMENT "Generating API documentation with doxygen" VERBATIM)

    add_dependencies(doxygen docdirs)

    ### README html ########################

    if(WIN32)
        set(README_LOCATION "${PROJECT_BINARY_DIR}/")
        set(README_LOCATION_PREFFIX "doc/")
        set(README_INSTALL_LOCATION ".")
    else()
        set(README_LOCATION "${PROJECT_BINARY_DIR}/doc/")
        set(README_INSTALL_LOCATION "${DOC_INSTALL_DIR}")
    endif()

    configure_file(doc/README.html.in ${README_LOCATION}/README.html @ONLY IMMEDIATE)

    ### ReadTheDocs ########################
#    if(NOT CHECK_DOCUMENTATION)
#        add_custom_target(readthedocs
#            COMMAND "${WGET_EXE}" "https://media.readthedocs.org/htmlzip/eprosima-fast-cdr/v${PROJECT_VERSION}/eprosima-fast-cdr.zip"
#            COMMAND "${UNZIP_EXE}" "eprosima-fast-cdr.zip" -d "${PROJECT_BINARY_DIR}/doc/"
#            COMMAND ${CMAKE_COMMAND} -E remove_directory "${PROJECT_BINARY_DIR}/doc/manual"
#            COMMAND ${CMAKE_COMMAND} -E rename "${PROJECT_BINARY_DIR}/doc/eprosima-fast-cdr-v${PROJECT_VERSION}" "${PROJECT_BINARY_DIR}/doc/manual"
#            COMMAND ${CMAKE_COMMAND} -E remove "eprosima-fast-cdr.zip"
#            )
#
#        add_dependencies(readthedocs docdirs)
#    endif()

    add_custom_target(doc ALL
        COMMENT "Generated project documentation" VERBATIM)

    add_dependencies(doc doxygen)
#    if(NOT CHECK_DOCUMENTATION)
#        add_dependencies(doc readthedocs)
#    endif()
endif()


###############################################################################
# Packaging
###############################################################################
# Install licenses
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION ${LICENSE_INSTALL_DIR}
    COMPONENT licenses
    )
set(CPACK_COMPONENT_LICENSES_HIDDEN 1)

set(CPACK_COMPONENTS_ALL headers licenses)

if(BUILD_DOCUMENTATION)

    # Instalation of doxygen files
    install(DIRECTORY ${PROJECT_BINARY_DIR}/doc/api_reference
        DESTINATION ${DOC_INSTALL_DIR}
        COMPONENT documentation
        )

    install(FILES "${README_LOCATION}/README.html"
        DESTINATION ${README_INSTALL_LOCATION}
        COMPONENT documentation
        )

#    if(NOT CHECK_DOCUMENTATION)
#        install(DIRECTORY ${PROJECT_BINARY_DIR}/doc/manual
#            DESTINATION ${DOC_INSTALL_DIR}
#            COMPONENT documentation
#            )
#    endif()

    ### Documentation component
    set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
    set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} documentation")

    set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} documentation)
endif()

if(MSVC OR MSVC_IDE)
    if(EPROSIMA_INSTALLER)
        set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL}
            libraries_i86Win32VS2010
            libraries_x64Win64VS2010
            libraries_i86Win32VS2013
            libraries_x64Win64VS2013
            libraries_i86Win32VS2015
            libraries_x64Win64VS2015
            )
    else()
        set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} libraries_${MSVC_ARCH})
    endif()
else()
    set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} libraries)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/packaging/eProsimaPackaging.cmake)
