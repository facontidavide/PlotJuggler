# Copyright (c) Meta Platforms, Inc. and affiliates.
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

# Finds liblz4.
#
# This module defines:
# LZ4_FOUND
# LZ4_INCLUDE_DIR
# LZ4_LIBRARY
#

if(UNIX)
  find_package(PkgConfig QUIET)
  pkg_search_module(PC_LZ4 lz4 liblz4)
endif()

find_path(LZ4_INCLUDE_DIR
  NAMES lz4.h
  HINTS
    ${PC_LZ4_INCLUDEDIR}
    ${PC_LZ4_INCLUDE_DIRS}
)

# find shared and static libraries
find_library(LZ4_SHARED_LIBRARY
  NAMES ${CMAKE_SHARED_LIBRARY_PREFIX}lz4${CMAKE_SHARED_LIBRARY_SUFFIX}
  HINTS
    ${PC_LZ4_LIBDIR}
    ${PC_LZ4_LIBRARY_DIRS}
  )

find_library(LZ4_STATIC_LIBRARY
  NAMES ${CMAKE_STATIC_LIBRARY_PREFIX}lz4${CMAKE_STATIC_LIBRARY_SUFFIX}
  HINTS
    ${PC_LZ4_LIBDIR}
    ${PC_LZ4_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4
  DEFAULT_MSG
  LZ4_INCLUDE_DIR
  LZ4_SHARED_LIBRARY
  LZ4_STATIC_LIBRARY
)

if(PC_LZ4_FOUND)
  message(STATUS "Found LZ4: shared=${LZ4_SHARED_LIBRARY}, static=${LZ4_STATIC_LIBRARY}")
else()
  message(WARNING "LZ4 not found")
  return()
endif()

# Create imported targets
if(NOT TARGET LZ4::lz4_shared AND LZ4_SHARED_LIBRARY)
  add_library(LZ4::lz4_shared SHARED IMPORTED GLOBAL)
  set_target_properties(LZ4::lz4_shared PROPERTIES
    IMPORTED_LOCATION           "${LZ4_SHARED_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
  )
endif()

if(NOT TARGET LZ4::lz4_static AND LZ4_STATIC_LIBRARY)
  add_library(LZ4::lz4_static STATIC IMPORTED GLOBAL)
  set_target_properties(LZ4::lz4_static PROPERTIES
    IMPORTED_LOCATION           "${LZ4_STATIC_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(LZ4_INCLUDE_DIR LZ4_SHARED_LIBRARY LZ4_STATIC_LIBRARY)
