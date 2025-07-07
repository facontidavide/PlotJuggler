
cmake_minimum_required(VERSION 3.16)
project(kissfft)

#
# CMake configuration options
# Principal datatype: double, float (default), int16_t, int32_t, simd

# Principal datatype of kissfft: double, float (default), int16_t, int32_t, simd
set(KISSFFT_DATATYPE "float")

#
# Print principal datatype
message(STATUS "Building KissFFT with datatype: ${KISSFFT_DATATYPE}")

#
# Detect C compiler and pass appropriate flags
#

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-fomit-frame-pointer -fPIC
        -W -Wall -Wcast-align -Wcast-qual -Wshadow -Wwrite-strings
        "$<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes;-Wmissing-prototypes;-Wnested-externs;-Wbad-function-cast>")
endif()

#
# Add GNUInstallDirs for GNU infrastructure before target)include_directories
#

if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|kFreeBSD|GNU)$" AND NOT CMAKE_CROSSCOMPILING)
    include(GNUInstallDirs)
endif()


#
# Declare kissfft library ( libkissfft.a / libkissfft-${KISSFFT_DATATYPE}.so.${MAKEFILE_EXTRACTED_VERSION} )
#

add_library(kissfft STATIC
  kiss_fft.c
  kfc.c
  kiss_fftnd.c
  kiss_fftndr.c
  kiss_fftr.c)

target_include_directories(kissfft PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${PKGINCLUDEDIR}>)

#
# Set compile definitions based on datatype and additional support flags
#

set(KISSFFT_COMPILE_DEFINITIONS)

#
# double / float
#

if(KISSFFT_DATATYPE MATCHES "^float$" OR KISSFFT_DATATYPE MATCHES "^double$")
    list(APPEND KISSFFT_COMPILE_DEFINITIONS kiss_fft_scalar=${KISSFFT_DATATYPE})
else()

    #
    # int16_t
    #

    if(KISSFFT_DATATYPE MATCHES "^int16_t$")
        list(APPEND KISSFFT_COMPILE_DEFINITIONS FIXED_POINT=16)
    else()

        #
        # int32_t
        #

        if(KISSFFT_DATATYPE MATCHES "^int32_t$")
            list(APPEND KISSFFT_COMPILE_DEFINITIONS FIXED_POINT=32)
        else()

            #
            # simd
            #

            if(KISSFFT_DATATYPE MATCHES "^simd$")
                list(APPEND KISSFFT_COMPILE_DEFINITIONS USE_SIMD)
                if (NOT MSVC)
                    target_compile_options(kissfft PRIVATE -msse)
                else()
                    target_compile_options(kissfft PRIVATE "/arch:SSE")
                endif()
            endif()
        endif()
    endif()
endif()


# Set library name, version, soversion and aliases

target_compile_definitions(kissfft PUBLIC ${KISSFFT_COMPILE_DEFINITIONS})
add_library(kissfft::kissfft ALIAS kissfft)

#
# Build with libm (-lm) on Linux and kFreeBSD
#

if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|kFreeBSD|GNU)$" AND NOT CMAKE_CROSSCOMPILING)
    target_link_libraries(kissfft PRIVATE m)
endif()


