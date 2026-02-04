# look for gtest from ROS, but do not fail if it is not found
find_package(gtest_vendor QUIET)
find_package(ament_cmake_gtest QUIET)

if(gtest_vendor_FOUND AND ament_cmake_gtest_FOUND)
    ament_add_gtest(datatamer_test
        dt_tests.cpp
        custom_types_tests.cpp
        parser_tests.cpp)

    target_include_directories(datatamer_test
        PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

    target_link_libraries(datatamer_test data_tamer)
else()
    if (NOT gtest_vendor_FOUND)
        message(STATUS "gtest vendor not found, falling back to system GTest")
    endif()
    if (NOT ament_cmake_gtest_FOUND)
        message(STATUS "ament_cmake_gtest not found, falling back to system GTest")
    endif()
    # we must have gtest either from the ROS vendor (above) or on the system
    find_package(GTest REQUIRED)

    include(GoogleTest)

    add_executable(datatamer_test
        dt_tests.cpp
        custom_types_tests.cpp
        parser_tests.cpp)
    gtest_discover_tests(datatamer_test DISCOVERY_MODE PRE_TEST)

    target_include_directories(datatamer_test
        PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

    target_link_libraries(datatamer_test data_tamer GTest::gtest_main)

    add_test(NAME datatamer_test COMMAND $<TARGET_FILE:datatamer_test>)
endif()
