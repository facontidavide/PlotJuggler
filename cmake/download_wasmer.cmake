
function(download_wasmer)

  if(TARGET wasmer::wasmer)
    return()
  endif()

  set(WASMER_VERSION "v7.0.1")
  set(WASMER_BASE_URL "https://github.com/wasmerio/wasmer/releases/download/${WASMER_VERSION}")

  if(WIN32)
    if(MINGW)
      message(STATUS "Downloading wasmer for MinGW")
      set(WASMER_URL "${WASMER_BASE_URL}/wasmer-windows-gnu64.tar.gz")
      set(WASMER_URL_HASH "4b7be5b79a127edf45bb79639f2747021363efb78f3e9698c9b052a4fbc3a4be")
    else()
      message(STATUS "Downloading wasmer for MSVC")
      set(WASMER_URL "${WASMER_BASE_URL}/wasmer-windows-amd64.tar.gz")
      set(WASMER_URL_HASH "7d76d4ce7e3ed40366bbd2b76c9039b78843a905df34c89fa5d6ffb227d78c77")
    endif()
  elseif(UNIX)
    if(APPLE)
      if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        message(STATUS "Downloading wasmer for macOS x86_64")
        set(WASMER_URL "${WASMER_BASE_URL}/wasmer-darwin-amd64.tar.gz")
        set(WASMER_URL_HASH "3a0f44a3aae570b0870d4573fa663c7f0c96a2f9550e38eb22c3be7c77658a1e")
      elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        message(STATUS "Downloading wasmer for macOS arm64")
        set(WASMER_URL "${WASMER_BASE_URL}/wasmer-darwin-arm64.tar.gz")
        set(WASMER_URL_HASH "3eff017389fb838b0b5af607a4d392edc6039e76343984fcd24307aa027d67ee")
      endif()
    else()
      if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        message(STATUS "Downloading wasmer for Linux x86_64")
        set(WASMER_URL "${WASMER_BASE_URL}/wasmer-linux-amd64.tar.gz")
        set(WASMER_URL_HASH "10a55885b11eb51b06bb24ff184facde8c2a83c252782a0c04e7a46926630d72")
      elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        message(STATUS "Downloading wasmer for Linux aarch64")
        set(WASMER_URL "${WASMER_BASE_URL}/wasmer-linux-aarch64.tar.gz")
        set(WASMER_URL_HASH "21d6968d33defa4a31d878022d261667a8fa8abbfe96007d5f4f28564b7fa372")
      endif()
    endif()
  endif()

  if(NOT WASMER_URL)
    message(ERROR "Unsupported platform for wasmer: ${CMAKE_SYSTEM_PROCESSOR}")
    return()
  endif()

  set(WASMER_STATIC_LIBRARY_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}wasmer${CMAKE_STATIC_LIBRARY_SUFFIX})
  message(STATUS "WASMER_URL: ${WASMER_URL}")

  cpmaddpackage(NAME wasmer
      URL ${WASMER_URL}
      URL_HASH SHA256=${WASMER_URL_HASH}
      DOWNLOAD_ONLY YES)

  add_library(wasmer::wasmer INTERFACE IMPORTED)

  if(NOT EXISTS ${wasmer_SOURCE_DIR}/lib/${WASMER_STATIC_LIBRARY_NAME})
    message(ERROR "wasmer library not found: ${wasmer_SOURCE_DIR}/lib/${WASMER_STATIC_LIBRARY_NAME}")
  endif()

  set(WASMER_LINK_LIBRARIES ${wasmer_SOURCE_DIR}/lib/${WASMER_STATIC_LIBRARY_NAME})
  set(WASMER_COMPILE_DEFINITIONS "")

  if(WIN32)
    list(APPEND WASMER_LINK_LIBRARIES ws2_32 advapi32 userenv ntdll bcrypt)
    list(APPEND WASMER_COMPILE_DEFINITIONS WASM_API_EXTERN= WASI_API_EXTERN=)
  else()
    list(APPEND WASMER_LINK_LIBRARIES pthread dl m)
  endif()

  set_target_properties(wasmer::wasmer PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${wasmer_SOURCE_DIR}/include"
      INTERFACE_LINK_LIBRARIES "${WASMER_LINK_LIBRARIES}"
      INTERFACE_COMPILE_DEFINITIONS "${WASMER_COMPILE_DEFINITIONS}")

  set(wasmer_FOUND TRUE CACHE BOOL "Whether wasmer was found or downloaded")

endfunction()
