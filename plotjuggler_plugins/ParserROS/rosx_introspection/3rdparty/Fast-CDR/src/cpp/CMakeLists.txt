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

include(${PROJECT_SOURCE_DIR}/cmake/dev/set_sources.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/dev/generate_msvc_libraries.cmake)

# Set source files
set_sources(
    Cdr.cpp
    FastCdr.cpp
    FastBuffer.cpp
    exceptions/Exception.cpp
    exceptions/NotEnoughMemoryException.cpp
    exceptions/BadParamException.cpp
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

# Create libraries
if(MSVC OR MSVC_IDE AND EPROSIMA_INSTALLER AND IS_TOP_LEVEL)
    generate_msvc_libraries(i86Win32VS2013)
    generate_msvc_libraries(x64Win64VS2013)
    generate_msvc_libraries(i86Win32VS2015)
    generate_msvc_libraries(x64Win64VS2015)
    generate_msvc_libraries(i86Win32VS2017)
    generate_msvc_libraries(x64Win64VS2017)
elseif(NOT EPROSIMA_INSTALLER)
    # Create library
    add_library(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
    set_common_compile_options(${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
    set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

    target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_NAME_UPPER}_SOURCE)

    # Define public headers
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
        )

    if(MSVC OR MSVC_IDE)
        set_target_properties(${PROJECT_NAME} PROPERTIES RELEASE_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
        set_target_properties(${PROJECT_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX -${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
        set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

        get_target_property(TARGET_TYPE ${PROJECT_NAME} TYPE)
        if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
            # Export symbols in DLL library
            target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_NAME_UPPER}_DYN_LINK)
            set_target_properties(${PROJECT_NAME} PROPERTIES
                PDB_NAME_DEBUG "${PROJECT_NAME}d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
                PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/symbols"
            )
        else()
            # Rename the library to have a "lib" before.
            set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME lib${PROJECT_NAME})
            set_target_properties(${PROJECT_NAME} PROPERTIES
                COMPILE_PDB_NAME_DEBUG "lib${PROJECT_NAME}d-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
                COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/symbols"
            )
        endif()

        # For installing purpose.
        if(EPROSIMA_INSTALLER_MINION)
            set(MSVCARCH_EXTENSION "_${MSVC_ARCH}")
            set(MSVCARCH_DIR_EXTENSION "/${MSVC_ARCH}")
            set(MSVCARCH_DIR_EXTENSION_EXT "/${MSVC_ARCH}")
            if(NOT TARGET_TYPE STREQUAL "SHARED_LIBRARY")
                set(MSVCARCH_DIR_EXTENSION_EXT "${MSVCARCH_DIR_EXTENSION_EXT}_static")
            endif()
        endif()
    endif()
endif()

###############################################################################
# Packaging
###############################################################################

# Install public headers
set_public_headers_directory(${PROJECT_SOURCE_DIR}/include ${PROJECT_NAME}
    DESTINATION ${INCLUDE_INSTALL_DIR}
    COMPONENT headers
    )

# Install config.h header
set_public_header(${PROJECT_BINARY_DIR}/include ${PROJECT_NAME} config.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/${PROJECT_NAME}
    COMPONENT headers
    )

set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers" PARENT_SCOPE)
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} C++ Headers" PARENT_SCOPE)

# Install sources
if(UNIX AND EPROSIMA_INSTALLER)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/cpp
        DESTINATION src
        COMPONENT sources
        FILES_MATCHING
        PATTERN "*.c"
        PATTERN "*.cpp"
        PATTERN "*.h"
        PATTERN "*.hpp"
        )

    install(DIRECTORY ${PROJECT_BINARY_DIR}/autotools/
        DESTINATION .
        USE_SOURCE_PERMISSIONS
        OPTIONAL
        COMPONENT sources
        )
endif()

# Install libraries

# Generate different target names depending on linking
get_target_property(TARGET_TYPE ${PROJECT_NAME} TYPE)
if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    set(FASTCDR_LINKING dynamic)
else()
    set(FASTCDR_LINKING static)
endif()

if(MSVC OR MSVC_IDE AND EPROSIMA_INSTALLER)
    if(IS_TOP_LEVEL)
        install_msvc_libraries(i86Win32VS2013)
        install_msvc_libraries(x64Win64VS2013)
        install_msvc_libraries(i86Win32VS2015)
        install_msvc_libraries(x64Win64VS2015)
        install_msvc_libraries(i86Win32VS2017)
        install_msvc_libraries(x64Win64VS2017)
    endif()

    install(FILES ${PROJECT_SOURCE_DIR}/cmake/packaging/windows/${PROJECT_NAME}-config.cmake
        DESTINATION ${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME}${MSVCARCH_DIR_EXTENSION_EXT}
        COMPONENT cmake
        )
elseif(NOT EPROSIMA_INSTALLER)
    install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-${FASTCDR_LINKING}-targets
        RUNTIME DESTINATION ${BIN_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
        COMPONENT libraries${MSVCARCH_EXTENSION}
        )

    export(TARGETS ${PROJECT_NAME} FILE ${PROJECT_BINARY_DIR}/cmake/config/${PROJECT_NAME}-${FASTCDR_LINKING}-targets.cmake)

    if(INSTALLER_PLATFORM)
        set(INSTALL_DESTINATION_PATH ${DATA_INSTALL_DIR}/${PROJECT_NAME}-${INSTALLER_PLATFORM}/cmake)
    else()
        set(INSTALL_DESTINATION_PATH ${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME}${MSVCARCH_DIR_EXTENSION_EXT})
    endif()

    install(EXPORT ${PROJECT_NAME}-${FASTCDR_LINKING}-targets
        DESTINATION ${INSTALL_DESTINATION_PATH}
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
                DESTINATION ${LIB_INSTALL_DIR}${MSVCARCH_DIR_EXTENSION}
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

    if(MSVCARCH_EXTENSION)
        string(TOUPPER "${MSVC_ARCH}" MSVC_ARCH_UPPER)
        set(CPACK_COMPONENT_LIBRARIES_${MSVC_ARCH_UPPER}_DISPLAY_NAME "${MSVC_ARCH}" PARENT_SCOPE)
        set(CPACK_COMPONENT_LIBRARIES_${MSVC_ARCH_UPPER}_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} libraries for platform ${MSVC_ARCH}" PARENT_SCOPE)
        set(CPACK_COMPONENT_LIBRARIES_${MSVC_ARCH_UPPER}_GROUP "Libraries" PARENT_SCOPE)
        set(CPACK_COMPONENT_GROUP_LIBRARIES_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} libraries" PARENT_SCOPE)
    else()
        set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries" PARENT_SCOPE)
        set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "eProsima ${PROJECT_NAME_LARGE} libraries" PARENT_SCOPE)
    endif()
endif()
