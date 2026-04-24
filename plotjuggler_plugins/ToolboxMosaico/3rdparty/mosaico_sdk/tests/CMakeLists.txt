# ===========================================================================
# Mosaico SDK integration test harness
#
# Gated by MOSAICO_INTEGRATION_TESTS=ON. Boots a real mosaicod server via
# tests/harness/setup_local_mosaico.sh (needs docker + openssl). Tests are
# NOT run by default ctest — use `ctest -L integration`.
# ===========================================================================

option(MOSAICO_INTEGRATION_TESTS "Build Mosaico SDK integration tests (requires docker)" OFF)

if(NOT MOSAICO_INTEGRATION_TESTS)
    return()
endif()

enable_testing()
find_package(GTest REQUIRED)

# Preflight: script must exist and be executable.
set(MOSAICO_HARNESS_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/harness/setup_local_mosaico.sh")
if(NOT EXISTS "${MOSAICO_HARNESS_SCRIPT}")
    message(FATAL_ERROR "[mosaico_sdk/tests] harness script missing: ${MOSAICO_HARNESS_SCRIPT}")
endif()

# Fail fast if docker/openssl absent at configure time (script runs --help
# which triggers its precondition check path — cheap, no side effects).
execute_process(
    COMMAND bash -c "command -v docker >/dev/null && command -v openssl >/dev/null"
    RESULT_VARIABLE _preflight_rc
)
if(NOT _preflight_rc EQUAL 0)
    message(WARNING "[mosaico_sdk/tests] docker or openssl missing — integration tests will fail at runtime")
endif()

# ---------------------------------------------------------------------------
# Harness library (fixture + timing)
# ---------------------------------------------------------------------------
add_library(mosaico_test_harness STATIC
    harness/server_fixture.cpp
    harness/timing.cpp
)
target_include_directories(mosaico_test_harness PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(mosaico_test_harness PUBLIC
    mosaico_sdk
    GTest::gtest
)
target_compile_definitions(mosaico_test_harness PUBLIC
    MOSAICO_HARNESS_SCRIPT="${MOSAICO_HARNESS_SCRIPT}"
)
target_compile_features(mosaico_test_harness PUBLIC cxx_std_17)

# ---------------------------------------------------------------------------
# Integration tests
# ---------------------------------------------------------------------------
add_executable(test_sequence_lifecycle
    integration/test_sequence_lifecycle.cpp
)
target_link_libraries(test_sequence_lifecycle PRIVATE
    mosaico_test_harness
    GTest::gtest_main
)
add_test(NAME SequenceLifecycle COMMAND test_sequence_lifecycle)
set_tests_properties(SequenceLifecycle PROPERTIES
    LABELS "integration"
    TIMEOUT 180
)

add_executable(test_list_and_pull
    integration/test_list_and_pull.cpp
)
target_link_libraries(test_list_and_pull PRIVATE
    mosaico_test_harness
    GTest::gtest_main
)
add_test(NAME ListAndPull COMMAND test_list_and_pull)
set_tests_properties(ListAndPull PROPERTIES
    LABELS "integration"
    TIMEOUT 180
)

add_executable(test_version integration/test_version.cpp)
target_link_libraries(test_version PRIVATE mosaico_test_harness GTest::gtest_main)
add_test(NAME Version COMMAND test_version)
set_tests_properties(Version PROPERTIES LABELS "integration" TIMEOUT 120)

add_executable(test_topic_delete integration/test_topic_delete.cpp)
target_link_libraries(test_topic_delete PRIVATE mosaico_test_harness GTest::gtest_main)
add_test(NAME TopicDelete COMMAND test_topic_delete)
set_tests_properties(TopicDelete PROPERTIES LABELS "integration" TIMEOUT 120)

add_executable(test_query integration/test_query.cpp)
target_link_libraries(test_query PRIVATE mosaico_test_harness GTest::gtest_main)
add_test(NAME Query COMMAND test_query)
set_tests_properties(Query PROPERTIES LABELS "integration" TIMEOUT 180)

add_executable(test_notifications integration/test_notifications.cpp)
target_link_libraries(test_notifications PRIVATE mosaico_test_harness GTest::gtest_main)
add_test(NAME Notifications COMMAND test_notifications)
set_tests_properties(Notifications PROPERTIES LABELS "integration" TIMEOUT 180)

add_executable(test_api_keys integration/test_api_keys.cpp)
target_link_libraries(test_api_keys PRIVATE mosaico_test_harness GTest::gtest_main)
add_test(NAME ApiKeys COMMAND test_api_keys)
set_tests_properties(ApiKeys PROPERTIES LABELS "integration" TIMEOUT 120)
