
add_executable(dt_benchmark data_tamer_benchmark.cpp)
target_include_directories(dt_benchmark
     PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_link_libraries(dt_benchmark data_tamer benchmark)
