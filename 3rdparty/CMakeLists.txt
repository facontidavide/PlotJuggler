

##### fastCDR ######

find_package(fastcdr QUIET)
if (NOT fastcdr_FOUND)
    CPMAddPackage("gh:eProsima/Fast-CDR@2.2.6")
endif()

##### backward_cpp ######

CPMAddPackage("gh:bombela/backward-cpp@1.6")

##### nlohmann_json ######


CPMAddPackage("gh:nlohmann/json@3.11.3")

##### MCAP + compression libs ######

find_package(zstd QUIET)
if (NOT zstd_FOUND)
    CPMAddPackage("gh:facebook/zstd@1.5.6")
endif()

find_package(lz4 QUIET)
if (NOT lz4_FOUND)
    CPMAddPackage("gh:lz4/lz4@1.10.0")
endif()

find_package(mcap QUIET)
if (NOT mcap_FOUND)

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
endif()

##### Lua + Sol2 ######

find_package(lua 5.4 QUIET)
if (NOT lua_FOUND)
    CPMAddPackage(
    NAME lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    VERSION 5.4.7
    DOWNLOAD_ONLY YES
    )
    # lua has no CMake support, so we create our own target
    if (lua_ADDED)
        file(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
        list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c" "${lua_SOURCE_DIR}/onelua.c")
        add_library(lua STATIC ${lua_sources})
        target_include_directories(lua SYSTEM PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)
        set_property(TARGET lua PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
 endif()

CPMAddPackage("gh:ThePhD/sol2@3.3.0")


##### data_tamer ######

CPMAddPackage(
    NAME data_tamer
    GITHUB_REPOSITORY PickNikRobotics/data_tamer
    GIT_TAG 0.9.3
    DOWNLOAD_ONLY YES
)
    
if (data_tamer_ADDED)
    # we are interested only in data_tamer_parser includes
    add_library(data_tamer_parser INTERFACE)
    target_include_directories(data_tamer_parser INTERFACE "${data_tamer_SOURCE_DIR}/data_tamer_cpp/include")
endif()
    
    
add_subdirectory( qwt/src )
add_subdirectory( QCodeEditor )
add_subdirectory( color_widgets )
add_subdirectory( Qt-Advanced-Docking )
