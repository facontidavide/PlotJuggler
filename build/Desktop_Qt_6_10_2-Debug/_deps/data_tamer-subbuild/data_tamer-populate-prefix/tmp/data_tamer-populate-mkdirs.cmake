# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-src")
  file(MAKE_DIRECTORY "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-src")
endif()
file(MAKE_DIRECTORY
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-build"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-subbuild/data_tamer-populate-prefix"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-subbuild/data_tamer-populate-prefix/tmp"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-subbuild/data_tamer-populate-prefix/src/data_tamer-populate-stamp"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-subbuild/data_tamer-populate-prefix/src"
  "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-subbuild/data_tamer-populate-prefix/src/data_tamer-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-subbuild/data_tamer-populate-prefix/src/data_tamer-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/alvaro/Proyects/DataStreamWebSocket/build/Desktop_Qt_6_10_2-Debug/_deps/data_tamer-subbuild/data_tamer-populate-prefix/src/data_tamer-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
