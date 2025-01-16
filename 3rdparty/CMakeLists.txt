
find_package(fastcdr QUIET)
if (NOT fastcdr_FOUND)
    CPMAddPackage(
        NAME fastcdr
        GITHUB_REPOSITORY eProsima/Fast-CDR
        GIT_TAG v2.2.6
    )
endif()

find_package(zstd QUIET)
if (NOT zstd_FOUND)
    CPMAddPackage(
        NAME zstd
        GITHUB_REPOSITORY facebook/zstd
        GIT_TAG v1.5.6
    )
endif()

find_package(lz4 QUIET)
if (NOT lz4_FOUND)
    CPMAddPackage(
        NAME lz4
        GITHUB_REPOSITORY lz4/lz4
        GIT_TAG v1.10.0
    )
endif()

CPMAddPackage(
    NAME mcap
    GITHUB_REPOSITORY foxglove/mcap
    GIT_TAG releases/cpp/v2.0.0
    DOWNLOAD_ONLY YES
)
# mcap has no CMake support, so we create our own target
if (mcap_ADDED)
    add_library(mcap INTERFACE)
    target_include_directories(mcap INTERFACE "${mcap_SOURCE_DIR}/cpp/mcap/include")
endif()

find_package(lua 5.4 QUIET)
if (NOT lua_FOUND)
    CPMAddPackage(
    NAME lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    VERSION 5.4.4
    DOWNLOAD_ONLY YES
    )
    # lua has no CMake support, so we create our own target
    if (lua_ADDED)
      FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
      list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c")
      add_library(lua STATIC ${lua_sources})
      target_include_directories(lua PUBLIC  $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)
      set_property(TARGET lua PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
 endif()


CPMAddPackage(
    NAME backward_cpp
    GITHUB_REPOSITORY bombela/backward-cpp
    GIT_TAG v1.6
)

CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG v3.11.3
)

CPMAddPackage(
    NAME sol2
    GITHUB_REPOSITORY ThePhD/sol2
    GIT_TAG v3.3.0
)

CPMAddPackage(
    NAME data_tamer
    GITHUB_REPOSITORY PickNikRobotics/data_tamer
    GIT_TAG 0.9.3
    DOWNLOAD_ONLY YES
    )
    
# we are interested only in data_tamer_parser includes
if (data_tamer_ADDED)
    add_library(data_tamer_parser INTERFACE)
    target_include_directories(data_tamer_parser INTERFACE "${data_tamer_SOURCE_DIR}/data_tamer_cpp/include")
endif()
    
    
add_subdirectory( qwt/src )
add_subdirectory( QCodeEditor )
add_subdirectory( color_widgets )
add_subdirectory( Qt-Advanced-Docking )
