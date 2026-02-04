
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(ZSTD REQUIRED)
find_package(LZ4 REQUIRED)

add_library(mcap_lib STATIC mcap.cpp)
target_link_libraries(mcap_lib PUBLIC ${LZ4_LIBRARY}  ${ZSTD_LIBRARY})
target_include_directories(mcap_lib
 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

