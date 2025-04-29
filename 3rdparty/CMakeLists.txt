
##### fastCDR ######
if(NOT TARGET fastcdr)
    CPMAddPackage(
        NAME fastcdr_imported
        GITHUB_REPOSITORY eProsima/Fast-CDR
        GIT_TAG v2.2.6
        OPTIONS "BUILD_SHARED_LIBS OFF"
    )
endif()

##### backward_cpp ######
CPMAddPackage("gh:bombela/backward-cpp@1.6")

##### nlohmann_json ######
CPMAddPackage("gh:nlohmann/json@3.11.3")

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
    GIT_TAG 1.0.1
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
