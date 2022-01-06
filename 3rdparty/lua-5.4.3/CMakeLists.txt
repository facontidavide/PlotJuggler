set(LUA_LIB_SRCS 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lapi.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lcode.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lctype.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ldebug.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ldo.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ldump.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lfunc.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lgc.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/llex.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lmem.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lobject.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lopcodes.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lparser.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lstate.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lstring.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ltable.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ltm.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lundump.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lvm.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lzio.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lauxlib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lbaselib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lcorolib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ldblib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liolib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lmathlib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loslib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lstrlib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ltablib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lutf8lib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loadlib.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/linit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lua.c)
set(LUA_LIB_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_compile_options(-fPIC)

add_library(lua_static STATIC ${LUA_LIB_SRCS})
target_include_directories(lua_static PUBLIC ${LUA_LIB_INCLUDE})

if ( NOT WIN32 )
    set(LUA_DEFINITIONS LUA_USE_POSIX)
endif()

if(EMSCRIPTEN)
    unset(LUA_DEFINITIONS)
endif()
target_compile_definitions(lua_static PUBLIC ${LUA_DEFINITIONS})

