function(find_or_download_lua)

    ##### Lua + Sol2 ######

    find_package(Lua QUIET)

    if(LUA_FOUND)
        message(STATUS "Found Lua in system")

        add_library(lua::lua INTERFACE IMPORTED)
        set_target_properties(lua::lua PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
        )
    elseif (NOT TARGET lua_static)
        message(STATUS "Lua not found, downloading")
        CPMAddPackage(
            NAME lua
            URL https://github.com/lua/lua/archive/refs/tags/v5.4.7.zip
            DOWNLOAD_ONLY YES
        )
        # lua has no CMake support, so we create our own target

        file(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
        list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c" "${lua_SOURCE_DIR}/onelua.c")
        add_library(lua_static STATIC ${lua_sources})
        target_include_directories(lua_static SYSTEM PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)
        set_property(TARGET lua_static PROPERTY POSITION_INDEPENDENT_CODE ON)

        set(LUA_FOUND TRUE FORCE)

        add_library(lua::lua INTERFACE IMPORTED)
        set_target_properties(lua::lua PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${lua_SOURCE_DIR}"
            INTERFACE_LINK_LIBRARIES lua_static
        )
    endif()


endfunction()
