cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(ulog_cpp
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "C++ library for handling ULog files"
)

include(CMakeDependentOption)

cmake_dependent_option(ULOG_CPP_INSTALL "Install the ulog_cpp folder to include/ during install process"
		ON "CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME" OFF)
cmake_dependent_option(ULOG_CPP_BUILD_TESTS "Build ulog_cpp tests" ON
		"CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME" OFF)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(ulog_cpp)

# Only create and install the config files if this is the main project
if(ULOG_CPP_INSTALL)
	include(GNUInstallDirs)
	include(CMakePackageConfigHelpers)

    # Create a config file that includes the export
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    # Create a version file
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Install the config files
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )


	add_compile_options(
		# Warnings
		-Wall
		-Wextra
		-Werror

		# disabled warnings
		-Wno-missing-field-initializers
		-Wno-unused-parameter
	)

	# compiler specific flags
	if (("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang"))
		add_compile_options(
			-fcolor-diagnostics # force color for clang (needed for clang + ccache)
			-fdiagnostics-absolute-paths # force absolute paths

			-Qunused-arguments

			-Wno-unknown-warning-option
			-Wno-unused-const-variable
		)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		add_compile_options(-fdiagnostics-color=always)
	endif()

	list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
	include(clang_format)

	add_subdirectory(examples)

	if (ULOG_CPP_BUILD_TESTS)
		add_subdirectory(test)
	endif()

endif()

target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>)

