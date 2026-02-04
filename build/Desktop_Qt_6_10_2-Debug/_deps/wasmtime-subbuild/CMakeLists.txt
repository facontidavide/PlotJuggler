# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.30.5)

# Reject any attempt to use a toolchain file. We must not use one because
# we could be downloading it here. If the CMAKE_TOOLCHAIN_FILE environment
# variable is set, the cache variable will have been initialized from it.
unset(CMAKE_TOOLCHAIN_FILE CACHE)
unset(ENV{CMAKE_TOOLCHAIN_FILE})

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(wasmtime-populate NONE)



include(ExternalProject)
ExternalProject_Add(wasmtime-populate
                     "URL" "https://github.com/bytecodealliance/wasmtime/releases/download/v37.0.0/wasmtime-v37.0.0-x86_64-linux-c-api.tar.xz" "URL_HASH" "SHA256=34749b52ef98e37bf7bf1076a6eaecb30f85a82aba78c7799e72ddacea2050fb"
                    SOURCE_DIR          "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-src"
                    BINARY_DIR          "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


