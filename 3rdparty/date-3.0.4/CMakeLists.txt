#[===================================================================[
   date library by Howard Hinnant

   CMake projects that wish to use this library should consider
   something like the following :

     include( FetchContent )
     FetchContent_Declare( date_src
       GIT_REPOSITORY https://github.com/HowardHinnant/date.git
       GIT_TAG        v3.0.1  # adjust tag/branch/commit as needed
     )
     FetchContent_MakeAvailable(date_src)
     ...
     target_link_libraries (my_target PRIVATE date::date)

#]===================================================================]

cmake_minimum_required( VERSION 3.7 )

project( date VERSION 3.0.4 )
set(ABI_VERSION 3) # used as SOVERSION, increment when ABI changes

get_directory_property( has_parent PARENT_DIRECTORY )

if (POLICY CMP0077)
    # Allow CMake 3.13+ to override options when using FetchContent / add_subdirectory.
    cmake_policy(SET CMP0077 NEW)
endif ()

# Override by setting on CMake command line.
set( CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard whose features are requested." )

option( USE_SYSTEM_TZ_DB "Use the operating system's timezone database" OFF )
option( MANUAL_TZ_DB "User will set TZ DB manually by invoking set_install in their code" OFF )
option( USE_TZ_DB_IN_DOT "Save the timezone database in the current folder" OFF )
option( BUILD_SHARED_LIBS  "Build a shared version of library" OFF )
option( ENABLE_DATE_TESTING "Enable unit tests" OFF )
option( DISABLE_STRING_VIEW "Disable string view" OFF )
option( COMPILE_WITH_C_LOCALE "define ONLY_C_LOCALE=1" OFF )
option( BUILD_TZ_LIB "build/install of TZ library" OFF )
option( ENABLE_DATE_INSTALL "Enable install" ON )

if( ENABLE_DATE_TESTING AND NOT BUILD_TZ_LIB )
    message(WARNING "Testing requested, but BUILD_TZ_LIB not ON - forcing the latter")
    set (BUILD_TZ_LIB ON CACHE BOOL "required for testing" FORCE)
endif( )

if( ENABLE_DATE_INSTALL )
  include( GNUInstallDirs )
endif( )

function( print_option OPT )
    if ( NOT DEFINED PRINT_OPTION_CURR_${OPT} OR ( NOT PRINT_OPTION_CURR_${OPT} STREQUAL ${OPT} ) )
        set( PRINT_OPTION_CURR_${OPT} ${${OPT}} CACHE BOOL "" )
        mark_as_advanced(PRINT_OPTION_CURR_${OPT})
        message( "# date: ${OPT} ${${OPT}}" )
    endif( )
endfunction( )

print_option( USE_SYSTEM_TZ_DB )
print_option( MANUAL_TZ_DB )
print_option( USE_TZ_DB_IN_DOT )
print_option( BUILD_SHARED_LIBS  )
print_option( ENABLE_DATE_TESTING )
print_option( DISABLE_STRING_VIEW )

#[===================================================================[
   date (header only) library
#]===================================================================]
add_library( date INTERFACE )
add_library( date::date ALIAS date )
target_include_directories( date INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> )
# adding header sources just helps IDEs
target_sources( date INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>/date/date.h
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>/date/solar_hijri.h
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>/date/islamic.h
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>/date/iso_week.h
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>/date/julian.h
)

set( APPLE_MOBILE_PLATFORM FALSE )
if ( IOS
    OR CMAKE_SYSTEM_NAME STREQUAL "iOS"
    OR CMAKE_SYSTEM_NAME STREQUAL "visionOS"
    OR CMAKE_SYSTEM_NAME STREQUAL "watchOS"
    OR CMAKE_SYSTEM_NAME STREQUAL "tvOS"
   )
    set( APPLE_MOBILE_PLATFORM TRUE )
endif()

set(TARGET_HEADERS
    include/date/date.h
    include/date/solar_hijri.h
    include/date/islamic.h
    include/date/iso_week.h
    include/date/julian.h
)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
    # public headers will get installed:
    set_target_properties( date PROPERTIES PUBLIC_HEADER "${TARGET_HEADERS}" )
endif ()

# These used to be set with generator expressions,
#
#   ONLY_C_LOCALE=$<IF:$<BOOL:${COMPILE_WITH_C_LOCALE}>,1,0>
#
# which expand in the output target file to, e.g.
#
#   ONLY_C_LOCALE=$<IF:$<BOOL:FALSE>,1,0>
#
# This string is then (somtimes?) not correctly interpreted.
if ( COMPILE_WITH_C_LOCALE )
  # To workaround libstdc++ issue https://github.com/HowardHinnant/date/issues/388
  target_compile_definitions( date INTERFACE ONLY_C_LOCALE=1 )
else()
  target_compile_definitions( date INTERFACE ONLY_C_LOCALE=0 )
endif()
if ( DISABLE_STRING_VIEW )
  target_compile_definitions( date INTERFACE HAS_STRING_VIEW=0 -DHAS_DEDUCTION_GUIDES=0 )
else()
  target_compile_definitions( date INTERFACE HAS_STRING_VIEW=1 )
endif()

#[===================================================================[
   tz (compiled) library
#]===================================================================]
if( BUILD_TZ_LIB )
    add_library( date-tz )
    target_compile_definitions( date-tz PRIVATE BUILD_TZ_LIB=1 )
    target_sources( date-tz
      PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>/date/tz.h
      PRIVATE
        include/date/tz_private.h
        src/tz.cpp )
    if ( APPLE_MOBILE_PLATFORM )
      target_sources( date-tz
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>/date/ios.h
        PRIVATE
          src/ios.mm )
    endif()
    add_library( date::date-tz ALIAS date-tz )
    target_link_libraries( date-tz PUBLIC date )
    target_include_directories( date-tz PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> )

    if ( USE_SYSTEM_TZ_DB OR MANUAL_TZ_DB )
      target_compile_definitions( date-tz PRIVATE AUTO_DOWNLOAD=0 HAS_REMOTE_API=0 )
    else()
      target_compile_definitions( date-tz PRIVATE AUTO_DOWNLOAD=1 HAS_REMOTE_API=1 )
    endif()

    if ( USE_SYSTEM_TZ_DB AND NOT WIN32 AND NOT MANUAL_TZ_DB )
      target_compile_definitions( date-tz PRIVATE INSTALL=. PUBLIC USE_OS_TZDB=1 )
    else()
      target_compile_definitions( date-tz PUBLIC USE_OS_TZDB=0 )
    endif()

    if ( WIN32 AND BUILD_SHARED_LIBS )
      target_compile_definitions( date-tz PUBLIC DATE_BUILD_DLL=1 )
    endif()

    set(TZ_HEADERS include/date/tz.h)

    if( APPLE_MOBILE_PLATFORM )
        list(APPEND TZ_HEADERS include/date/ios.h)
    endif( )
    set_target_properties( date-tz PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        PUBLIC_HEADER "${TZ_HEADERS}"
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${ABI_VERSION}" )
    if( NOT MSVC )
        find_package( Threads )
        target_link_libraries( date-tz PUBLIC Threads::Threads )
    endif( )
    if( NOT USE_SYSTEM_TZ_DB AND NOT MANUAL_TZ_DB )
        find_package( CURL REQUIRED )
        target_link_libraries( date-tz PRIVATE CURL::libcurl )
    endif( )
endif( )

#[===================================================================[
   installation
#]===================================================================]
if( ENABLE_DATE_INSTALL )
  set( version_config "${CMAKE_CURRENT_BINARY_DIR}/dateConfigVersion.cmake" )

  include( CMakePackageConfigHelpers )
  write_basic_package_version_file( "${version_config}"
      VERSION ${PROJECT_VERSION}
      COMPATIBILITY SameMajorVersion )

  install( TARGETS date
      EXPORT dateConfig
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/date )
  export( TARGETS date NAMESPACE date:: FILE dateTargets.cmake )
  if (CMAKE_VERSION VERSION_LESS 3.15)
      install(
          FILES ${TARGET_HEADERS}
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/date )
  endif ()

  if( BUILD_TZ_LIB )
      install( TARGETS date-tz
          EXPORT dateConfig
          PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/date
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )  # This is for Windows
      export( TARGETS date-tz NAMESPACE date:: APPEND FILE dateTargets.cmake )
  endif( )

  if( WIN32 AND NOT CYGWIN)
      set( CONFIG_LOC CMake )
  else( )
      set( CONFIG_LOC "${CMAKE_INSTALL_LIBDIR}/cmake/date" )
  endif( )
  install( EXPORT dateConfig
    FILE dateTargets.cmake
    NAMESPACE date::
    DESTINATION ${CONFIG_LOC} )
  install (
    FILES cmake/dateConfig.cmake "${version_config}"
    DESTINATION ${CONFIG_LOC})
endif( )

#[===================================================================[
   testing
#]===================================================================]
if( ENABLE_DATE_TESTING )
    enable_testing( )

    add_custom_target( testit COMMAND ${CMAKE_CTEST_COMMAND} )
    add_dependencies( testit date-tz )

    function( add_pass_tests TEST_GLOB TEST_PREFIX )
        file( GLOB_RECURSE FILENAMES ${TEST_GLOB} )

        foreach( TEST_FILE ${FILENAMES} )
            get_filename_component( TEST_NAME ${TEST_FILE} NAME_WE )
            get_filename_component( TEST_EXT ${TEST_FILE} EXT )
            if( NOT ${TEST_EXT} STREQUAL ".fail.cpp" )
                set( PREFIX "${TEST_PREFIX}_pass_${TEST_NAME}" )
                set( BIN_NAME ${PREFIX}_bin )
                set( TST_NAME ${PREFIX}_test )
                add_executable( ${BIN_NAME} EXCLUDE_FROM_ALL ${TEST_FILE} )
                add_test( ${TST_NAME} ${BIN_NAME} )
                target_link_libraries( ${BIN_NAME} date-tz )
                # HACK: because the test files don't use FQ includes:
                target_include_directories( ${BIN_NAME} PRIVATE include/date )
                add_dependencies( testit ${BIN_NAME} )
            endif( )
        endforeach( )
    endfunction( )

    function( add_fail_tests TEST_GLOB TEST_PREFIX )
        file( GLOB_RECURSE FILENAMES ${TEST_GLOB} )

        foreach( TEST_FILE ${FILENAMES} )
            get_filename_component( TEST_NAME ${TEST_FILE} NAME_WE )
            get_filename_component( TEST_EXT ${TEST_FILE} EXT )

            set( TEST_TYPE "_fail" )

            set( PREFIX "${TEST_PREFIX}_fail_${TEST_NAME}" )
            set( BIN_NAME ${PREFIX}_bin )
            set( TST_NAME ${PREFIX}_test )

            set( TEST_BIN_NAME ${CMAKE_BINARY_DIR}/${BIN_NAME} )
            add_custom_target( ${BIN_NAME}
                COMMAND
                    ${PROJECT_SOURCE_DIR}/compile_fail.sh
                    ${TEST_BIN_NAME}
                    ${CMAKE_CXX_COMPILER}
                    -std=c++14
                    -L${CMAKE_BINARY_DIR}/
                    -ldate-tz
                    -I${PROJECT_SOURCE_DIR}/include
                    -I${PROJECT_SOURCE_DIR}/include/date
                    -o ${BIN_NAME}
                    ${TEST_FILE}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT ${TST_NAME} )
            add_test( ${TST_NAME} "${PROJECT_SOURCE_DIR}/test_fail.sh" ${CMAKE_BINARY_DIR}/${BIN_NAME} WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/" )
            #set_tests_properties( ${TST_NAME} PROPERTIES WILL_FAIL TRUE)
            add_dependencies( testit ${BIN_NAME} )
        endforeach( )
    endfunction( )

    file( GLOB children RELATIVE "${PROJECT_SOURCE_DIR}/test" "${PROJECT_SOURCE_DIR}/test/*" )
    foreach( child ${children} )
        if( IS_DIRECTORY "${PROJECT_SOURCE_DIR}/test/${child}" )
            set( CUR_FOLDER "${PROJECT_SOURCE_DIR}/test/${child}" )
            add_pass_tests( "${CUR_FOLDER}/*.cpp" ${child} )
            if( NOT WIN32 )
                add_fail_tests( "${CUR_FOLDER}/*.fail.cpp" ${child} )
            endif( )
        endif( )
    endforeach( )
endif( )
