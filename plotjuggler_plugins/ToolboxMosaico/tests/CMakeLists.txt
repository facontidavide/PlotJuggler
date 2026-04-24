find_package(GTest REQUIRED)

add_executable(mosaico_unit_tests
    core/time_format_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/time_format.cpp
)
target_include_directories(mosaico_unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(mosaico_unit_tests PRIVATE GTest::gtest_main)
add_test(NAME mosaico_unit_tests COMMAND mosaico_unit_tests)

include(GoogleTest)

add_executable(ToolboxMosaico_tests test_tls_utils.cpp)
target_link_libraries(ToolboxMosaico_tests PRIVATE GTest::gtest_main Qt5::Core)
target_include_directories(ToolboxMosaico_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
gtest_discover_tests(ToolboxMosaico_tests)

target_sources(ToolboxMosaico_tests PRIVATE test_upload_utils.cpp)

target_sources(ToolboxMosaico_tests PRIVATE
    ui/server_history_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/server_history.cpp
)
target_include_directories(ToolboxMosaico_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# SDK-level ontology registry coverage — links against mosaico_sdk to exercise
# the real built-in registry and its builders.
add_executable(ontology_registry_tests ontology_registry_test.cpp)
target_link_libraries(ontology_registry_tests PRIVATE
    GTest::gtest_main
    mosaico_sdk
)
gtest_discover_tests(ontology_registry_tests)

# Offline MCAP upload-pipeline benchmark. Not a test — run it by hand:
#   cmake-build-debug/bin/mcap_bench <file.mcap> [<more.mcap> ...]
add_executable(mcap_bench mcap_bench.cpp)
target_link_libraries(mcap_bench PRIVATE mosaico_sdk)
