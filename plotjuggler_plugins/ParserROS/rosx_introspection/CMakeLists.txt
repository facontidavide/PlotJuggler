cmake_minimum_required(VERSION 3.10)

project(rosx_introspection  LANGUAGES C CXX VERSION 2.0.0)

# Build flags
#---- Enable C++17 ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT fastcdr_FOUND )
    message(STATUS "[FastCdr] not found, create shared libraries")
    # Override Fast-CDR option: compile as static lib
    SET(BUILD_SHARED_LIBS OFF CACHE BOOL "Create shared libraries by default")
    add_subdirectory(3rdparty/Fast-CDR)
    include_directories(3rdparty/Fast-CDR/include)
else()
    # Mention that FastCdr has been found on system
    message(STATUS "[FastCdr] found, version: ${fastcdr_VERSION}")
endif()

###############################################
## Declare a C++ library
###############################################
add_library(rosx_introspection STATIC
    ${SRC_FILES}
    src/ros_type.cpp
    src/ros_field.cpp
    src/stringtree_leaf.cpp
    src/ros_message.cpp
    src/ros_parser.cpp
    src/deserializer.cpp
    )
target_link_libraries(rosx_introspection
    fastcdr )

target_include_directories(rosx_introspection PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

