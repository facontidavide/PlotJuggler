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
cmake_minimum_required(VERSION 3.16)

# Set CMAKE_BUILD_TYPE to Release by default.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set BUILD_TESTING to OFF by default.
if(NOT BUILD_TESTING)
    message(STATUS "Tests not compiled by default")
    set(BUILD_TESTING OFF CACHE BOOL "Enable testing" FORCE)
endif()

###############################################################################
# Project                                                                     #
###############################################################################
project(fastcdr VERSION 2.2.1 LANGUAGES CXX)

set(PROJECT_NAME_STYLED "FastCDR")
set(PROJECT_NAME_LARGE "Fast CDR")
string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
set(${PROJECT_NAME}_DESCRIPTION_SUMMARY "C++ library for serialize using CDR serialization")
set(${PROJECT_NAME}_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} library provides two serialization mechanisms. One is the standard CDR serialization mechanism, while the other is a faster implementation that modifies the standard.")

message(STATUS "Configuring ${PROJECT_NAME_LARGE}")
message(STATUS "Version: ${PROJECT_VERSION}")

###############################################################################
# CCache on Windows on CI
###############################################################################
if (MSVC AND CMAKE_CXX_COMPILER_LAUNCHER STREQUAL "ccache")
    foreach(config DEBUG RELWITHDEBINFO)
        foreach(lang C CXX)
            set(flags_var "CMAKE_${lang}_FLAGS_${config}")
            string(REPLACE "/Zi" "/Z7" ${flags_var} "${${flags_var}}")
            set(${flags_var} "${${flags_var}}")
        endforeach()
    endforeach()
endif()

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
# Test system configuration
###############################################################################
include(${PROJECT_SOURCE_DIR}/cmake/common/check_configuration.cmake)

set(FORCE_CXX "11" CACHE STRING "C++ standard fulfillment selection")
check_stdcxx(${FORCE_CXX})

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
set(LICENSE_INSTALL_DIR ${DATA_INSTALL_DIR}/${PROJECT_NAME} CACHE PATH "Installation directory for licenses")

###############################################################################
# Compile library.
###############################################################################
add_subdirectory(src/cpp)

###############################################################################
# Documentation
###############################################################################
# Add an option to toggle the generation of the API documentation.
option(BUILD_DOCUMENTATION "Use doxygen to create product documentation" OFF)
option(CHECK_DOCUMENTATION "Use doxygen to check code documentation" OFF)
option(GENERATE_PDF_DOC "Use doxygen to generate API Reference PDF" OFF)

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
    if(GENERATE_PDF_DOC)
        set(GENERATE_LATEX YES)
    else()
        set(GENERATE_LATEX NO)
    endif()
    # Configure the template doxyfile for or specific project
    configure_file(doxyfile.in ${PROJECT_BINARY_DIR}/doxyfile @ONLY IMMEDIATE)
    # Add custom target to run doxygen when ever the project is build
    add_custom_target(doxygen
        COMMAND "${DOXYGEN_EXECUTABLE}" "${PROJECT_BINARY_DIR}/doxyfile"
        SOURCES "${PROJECT_BINARY_DIR}/doxyfile"
        COMMENT "Generating API documentation with doxygen" VERBATIM)

    add_dependencies(doxygen docdirs)

    if(GENERATE_PDF_DOC)
        add_custom_target(make_pdf ALL
            COMMAND "make" "pdf"
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/doxygen/latex
            COMMENT "Generating API reference PDF" VERBATIM)

        add_dependencies(make_pdf doxygen)
    endif()

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

    add_custom_target(doc ALL
        COMMENT "Generated project documentation" VERBATIM)

    add_dependencies(doc doxygen)
endif()


###############################################################################
# Packaging
###############################################################################
# Install licenses
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION ${LICENSE_INSTALL_DIR}
    COMPONENT licenses
    )

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

    if(GENERATE_PDF_DOC)
        install(FILES "${PROJECT_BINARY_DIR}/doxygen/latex/refman.pdf"
        DESTINATION ${DOC_INSTALL_DIR}
        RENAME fastcdr-${PROJECT_VERSION}-api-reference.pdf
        COMPONENT documentation)
    endif()
endif()
