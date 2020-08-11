if(NOT MSVC)
    add_compile_options(-fno-access-control)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-thread-safety)
    endif()

    add_executable(parse_test parse_test.cpp)
    target_link_libraries(parse_test simple-websocket-server)
    add_test(parse_test parse_test)
endif()

add_executable(crypto_test crypto_test.cpp)
target_link_libraries(crypto_test simple-websocket-server)
add_test(crypto_test crypto_test)

add_executable(io_test io_test.cpp)
target_link_libraries(io_test simple-websocket-server)
add_test(io_test io_test)
