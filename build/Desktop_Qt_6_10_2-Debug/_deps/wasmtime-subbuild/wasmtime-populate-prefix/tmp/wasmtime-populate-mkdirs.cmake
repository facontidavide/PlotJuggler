# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-src")
  file(MAKE_DIRECTORY "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-src")
endif()
file(MAKE_DIRECTORY
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-build"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-subbuild/wasmtime-populate-prefix"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-subbuild/wasmtime-populate-prefix/tmp"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src/wasmtime-populate-stamp"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src/wasmtime-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src/wasmtime-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/wasmtime-subbuild/wasmtime-populate-prefix/src/wasmtime-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
