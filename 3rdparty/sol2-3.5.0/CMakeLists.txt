

add_library(sol2 INTERFACE )
target_include_directories(sol2 INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

add_library(sol2::sol2 ALIAS sol2)

 install(TARGETS sol2 EXPORT plotjugglerTargets )