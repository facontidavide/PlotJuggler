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

file(GLOB_RECURSE ALL_HEADERS
    ${PROJECT_SOURCE_DIR}/include/**/*.h
    ${PROJECT_SOURCE_DIR}/include/**/*.hpp
    ${PROJECT_SOURCE_DIR}/src/cpp/**/*.h
    ${PROJECT_SOURCE_DIR}/src/cpp/**/*.hpp
    )

# Set source files
set(${PROJECT_NAME}_source_files
    ${ALL_HEADERS}

    Cdr.cpp
    CdrSizeCalculator.cpp
    FastCdr.cpp
    FastBuffer.cpp
    exceptions/BadOptionalAccessException.cpp
    exceptions/BadParamException.cpp
    exceptions/Exception.cpp
    exceptions/LockedExternalAccessException.cpp
    exceptions/NotEnoughMemoryException.cpp
    FastCdr.rc
    )

configure_file(${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/config.h.in
    ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}/config.h)

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

# Create library
add_library(${PROJECT_NAME} ${${PROJECT_NAME}_source_files})
set_common_compile_options(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    )

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    ${PROJECT_NAME_UPPER}_SOURCE
    INTERFACE
    $<$<BOOL:${WIN32}>:${PROJECT_NAME_UPPER}_NO_LIB>
    PUBLIC
    $<$<STREQUAL:$<TARGET_PROPERTY:${PROJECT_NAME},TYPE>,SHARED_LIBRARY>:${PROJECT_NAME_UPPER}_DYN_LINK>
    )

# Define public headers
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
    )

if(MSVC OR MSVC_IDE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RELEASE_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        RELWITHDEBINFO_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        DEBUG_POSTFIX d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        )

    get_target_property(TARGET_TYPE ${PROJECT_NAME} TYPE)
    if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
        # Export symbols in DLL library
        set_target_properties(${PROJECT_NAME} PROPERTIES
            PDB_NAME_DEBUG ${PROJECT_NAME}d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
            PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/symbols"
            )
    else()
        # Rename the library to have a "lib" before.
        set_target_properties(${PROJECT_NAME} PROPERTIES
            OUTPUT_NAME lib${PROJECT_NAME}
            COMPILE_PDB_NAME_DEBUG lib${PROJECT_NAME}d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
            COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/symbols"
            )
    endif()
endif()

###############################################################################
# Packaging
###############################################################################

# Install public headers
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}
    DESTINATION ${INCLUDE_INSTALL_DIR}
    COMPONENT headers
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.ipp"
    )

# Install config.h header
install(FILES ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}/config.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/${PROJECT_NAME}
    COMPONENT headers
    )

# Install libraries
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    COMPONENT libraries
    )

# Generate different target names depending on linking
get_target_property(TARGET_TYPE ${PROJECT_NAME} TYPE)
if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    set(FASTCDR_LINKING shared)
else()
    set(FASTCDR_LINKING static)
endif()

if(INSTALLER_PLATFORM)
    set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${PROJECT_NAME}-${INSTALLER_PLATFORM}/cmake)
else()
    set(INSTALL_DESTINATION_PATH ${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME})
endif()

install(EXPORT ${PROJECT_NAME}-targets
    DESTINATION ${INSTALL_DESTINATION_PATH}
    FILE ${PROJECT_NAME}-${FASTCDR_LINKING}-targets.cmake
    COMPONENT cmake
    )

if(MSVC OR MSVC_IDE)
    # first try dll symbols
    get_target_property(PDB_FILE ${PROJECT_NAME} PDB_NAME_DEBUG)
    if(PDB_FILE)
        get_target_property(PDB_DIR ${PROJECT_NAME} PDB_OUTPUT_DIRECTORY_DEBUG)
        set(PDB_FILE "${PDB_DIR}/${PDB_FILE}.pdb")
    else()
        # fallback to static lib symbols
        get_target_property(PDB_FILE ${PROJECT_NAME} COMPILE_PDB_NAME_DEBUG)
        if(PDB_FILE)
            get_target_property(PDB_DIR ${PROJECT_NAME} COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG)
            set(PDB_FILE "${PDB_DIR}/${PDB_FILE}.pdb")
        endif()
    endif()

    # install symbols if any
    if(PDB_FILE)
        install(FILES ${PDB_FILE}
            DESTINATION ${LIB_INSTALL_DIR}
            COMPONENT symbols
            CONFIGURATIONS Debug
            )
    endif()
endif()

###############################################################################
# Create CMake package config file
###############################################################################
include(CMakePackageConfigHelpers)

if(BUILD_SHARED_LIBS)
    set(FASTRTPS_PACKAGE_OPT_BIN_DIR_CONDITION "if(MSVC OR MSVC_IDE)")
else()
    set(FASTRTPS_PACKAGE_OPT_BIN_DIR_CONDITION "if(0)")
endif()

configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/packaging/Config.cmake.in
    ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${INSTALL_DESTINATION_PATH}
    PATH_VARS BIN_INSTALL_DIR INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
    )
write_basic_package_version_file(${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    )
install(FILES ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-config.cmake
    ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-config-version.cmake
    DESTINATION ${INSTALL_DESTINATION_PATH}
    COMPONENT cmake
    )
