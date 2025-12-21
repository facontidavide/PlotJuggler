

function(download_wasmtime)

  find_package(wasmtime QUIET CONFIG)

  if(wasmtime_FOUND)
    message(STATUS "Found wasmtime in system")
  elseif(NOT TARGET wasmtime)

    SET(WASMTIME_VERSION "v37.0.0")
    set(WASMTIME_BASE_URL "https://github.com/bytecodealliance/wasmtime/releases/download/${WASMTIME_VERSION}")

    if(WIN32)
        # Distinguish MSVC-driven build from MinGW
        if(MINGW)
            message(STATUS "Downloading Wasmtime for MinGW")
            set(WASMTIME_URL "${WASMTIME_BASE_URL}/wasmtime-${WASMTIME_VERSION}-x86_64-mingw-c-api.zip")
            set(WASMTIME_URL_HASH "2bffd359ddadf1076750d376f14a9599d231aff6e4b5d31f00eb05f4fe7cdbbe")
        else()
            message(STATUS "Downloading Wasmtime for MSVC")
            set(WASMTIME_URL "${WASMTIME_BASE_URL}/wasmtime-${WASMTIME_VERSION}-x86_64-windows-c-api.zip")
            set(WASMTIME_URL_HASH "7c368f45e33b89e3a826fff95be2282b64f4644e2ad6a31b27a546dad46a4bcb")
        endif()
    elseif(UNIX)
        if(APPLE)
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
                message(STATUS "Downloading Wasmtime for MacOS x86_64")
                set(WASMTIME_URL "${WASMTIME_BASE_URL}/wasmtime-${WASMTIME_VERSION}-x86_64-macos-c-api.tar.xz")
                set(WASMTIME_URL_HASH "f36607fb14d1f5392591aa756904c603f20fc642e26f0d44d7979053144ae695")
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
                message(STATUS "Downloading Wasmtime for MacOS arm64")
                set(WASMTIME_URL "${WASMTIME_BASE_URL}/wasmtime-${WASMTIME_VERSION}-aarch64-macos-c-api.tar.xz")
                set(WASMTIME_URL_HASH "e17b8abce4ab187054a5b26feb84b54f4a5985e660cace6cd70f8d0b8eab1468")
            endif()
        else()
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
                message(STATUS "Downloading Wasmtime for Linux x86_64")
                set(WASMTIME_URL "${WASMTIME_BASE_URL}/wasmtime-${WASMTIME_VERSION}-x86_64-linux-c-api.tar.xz")
                set(WASMTIME_URL_HASH "34749b52ef98e37bf7bf1076a6eaecb30f85a82aba78c7799e72ddacea2050fb")
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
                message(STATUS "Downloading Wasmtime for Linux arm64")
                set(WASMTIME_URL "${WASMTIME_BASE_URL}/wasmtime-${WASMTIME_VERSION}-aarch64-linux-c-api.tar.xz")
                set(WASMTIME_URL_HASH "b9264a2d4927ed2a38a51e5970dcc9d6f50d67e54753bd707970fdac6310b6fb")
            endif()
        endif()
    endif()

    if(NOT WASMTIME_URL)
        message(ERROR "Unsupported platform for wasmtime: ${CMAKE_SYSTEM_PROCESSOR}")
        return()
    endif()

    SET(WASMTIME_STATIC_LIBRARY_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}wasmtime${CMAKE_STATIC_LIBRARY_SUFFIX})
    message(STATUS "WASMTIME_URL: ${WASMTIME_URL}")
    message(STATUS "WASMTIME_STATIC_LIBRARY_NAME: ${WASMTIME_STATIC_LIBRARY_NAME}")

    cpmaddpackage(NAME wasmtime
        URL ${WASMTIME_URL}
        URL_HASH SHA256=${WASMTIME_URL_HASH}
        DOWNLOAD_ONLY YES )

    add_library(wasmtime::wasmtime INTERFACE IMPORTED)

    # check that the library is present:
    if(NOT EXISTS ${wasmtime_SOURCE_DIR}/lib/${WASMTIME_STATIC_LIBRARY_NAME})
        message(ERROR "Wasmtime library not found: ${wasmtime_SOURCE_DIR}/lib/${WASMTIME_STATIC_LIBRARY_NAME}")
    endif()

    # On Windows, Wasmtime requires additional system libraries and compile definitions for static linking
    set(WASMTIME_LINK_LIBRARIES ${wasmtime_SOURCE_DIR}/lib/${WASMTIME_STATIC_LIBRARY_NAME})
    set(WASMTIME_COMPILE_DEFINITIONS "")

    if(WIN32)
        # Add Windows system library dependencies
        list(APPEND WASMTIME_LINK_LIBRARIES
            ws2_32
            advapi32
            userenv
            ntdll
            shell32
            ole32
            bcrypt
        )
        # Add compile definitions to disable dllimport for static linking
        list(APPEND WASMTIME_COMPILE_DEFINITIONS
            WASM_API_EXTERN=
            WASI_API_EXTERN=
        )
    endif()

    set_target_properties(
      wasmtime::wasmtime
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${wasmtime_SOURCE_DIR}/include
                 INTERFACE_LINK_LIBRARIES "${WASMTIME_LINK_LIBRARIES}"
                 INTERFACE_COMPILE_DEFINITIONS "${WASMTIME_COMPILE_DEFINITIONS}")

    set(wasmtime_FOUND TRUE CACHE BOOL "Whether wasmtime was found or downloaded")

  endif()

endfunction()
