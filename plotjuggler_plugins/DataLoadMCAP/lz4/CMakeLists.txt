
PROJECT(lz4 LANGUAGES C CXX VERSION 1.9.3)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(lz4_static STATIC
    lz4.c
    lz4hc.c
    lz4frame.c
    xxhash.c
    )



