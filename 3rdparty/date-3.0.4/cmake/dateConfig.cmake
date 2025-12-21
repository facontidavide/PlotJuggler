include( CMakeFindDependencyMacro )
include( "${CMAKE_CURRENT_LIST_DIR}/dateTargets.cmake" )
if( NOT MSVC AND TARGET date::date-tz )
    find_dependency( Threads REQUIRED)
    get_target_property( _tzill date::date-tz  INTERFACE_LINK_LIBRARIES )
    if( _tzill AND "${_tzill}" MATCHES "libcurl" )
        find_dependency( CURL )
    endif( )
endif( )


