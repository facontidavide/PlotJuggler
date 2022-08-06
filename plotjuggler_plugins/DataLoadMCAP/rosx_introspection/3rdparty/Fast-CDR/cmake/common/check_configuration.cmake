# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

macro(check_stdcxx)
    # Check C++11
    include(CheckCXXCompilerFlag)
    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "QCC")
        check_cxx_compiler_flag(-std=c++14 SUPPORTS_CXX14)
        set(HAVE_CXX14 0)
        set(HAVE_CXX1Y 0)
        set(HAVE_CXX11 0)
        set(HAVE_CXX0X 0)
        if(SUPPORTS_CXX14)
            add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++14>)
            set(HAVE_CXX14 1)
            set(HAVE_CXX1Y 1)
            set(HAVE_CXX11 1)
            set(HAVE_CXX0X 1)
        else()
            check_cxx_compiler_flag(-std=c++1y SUPPORTS_CXX1Y)
            if(SUPPORTS_CXX1Y)
                add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++1y>)
                set(HAVE_CXX1Y 1)
                set(HAVE_CXX11 1)
                set(HAVE_CXX0X 1)
            else()
                check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
                if(SUPPORTS_CXX11)
                    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++11>)
                    set(HAVE_CXX11 1)
                    set(HAVE_CXX0X 1)
                else()
                    check_cxx_compiler_flag(-std=c++0x SUPPORTS_CXX0X)
                    if(SUPPORTS_CXX0X)
                        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++0x>)
                        set(HAVE_CXX0X 1)
                    else()
                        set(HAVE_CXX0X 0)
                    endif()
                endif()
            endif()
        endif()
    elseif(MSVC OR MSVC_IDE)
        set(HAVE_CXX11 1)
        set(HAVE_CXX0X 1)
    else()
        set(HAVE_CXX11 0)
        set(HAVE_CXX0X 0)
    endif()
endmacro()

macro(check_compile_feature)
    # Check constexpr
    list(FIND CMAKE_CXX_COMPILE_FEATURES "cxx_constexpr" CXX_CONSTEXPR_SUPPORTED)
    if(${CXX_CONSTEXPR_SUPPORTED} GREATER -1)
        set(HAVE_CXX_CONSTEXPR 1)
    else()
        set(HAVE_CXX_CONSTEXPR 0)
    endif()
endmacro()

macro(check_type_sizes)
    include(CheckTypeSize)
    check_type_size("long double" TYPE_LONG_DOUBLE)
    set(FASTCDR_SIZEOF_LONG_DOUBLE ${TYPE_LONG_DOUBLE})
    check_type_size("__float128" TYPE_FLOAT128)
    if(HAVE_TYPE_FLOAT128)
        set(FASTCDR_HAVE_FLOAT128 1)
    else()
        set(FASTCDR_HAVE_FLOAT128 0)
    endif()
endmacro()

macro(check_endianness)
    # Test endianness
    include(TestBigEndian)
    test_big_endian(BIG_ENDIAN)
    set(FASTCDR_IS_BIG_ENDIAN_TARGET ${BIG_ENDIAN})
endmacro()

macro(check_msvc_arch)
    if(MSVC_VERSION EQUAL 1700)
        if(CMAKE_CL_64)
            set(MSVC_ARCH "x64Win64VS2012")
        else()
            set(MSVC_ARCH "i86Win32VS2012")
        endif()
    elseif(MSVC_VERSION EQUAL 1800)
        if(CMAKE_CL_64)
            set(MSVC_ARCH "x64Win64VS2013")
        else()
            set(MSVC_ARCH "i86Win32VS2013")
        endif()
    elseif(MSVC_VERSION EQUAL 1900)
        if(CMAKE_CL_64)
            set(MSVC_ARCH "x64Win64VS2015")
        else()
            set(MSVC_ARCH "i86Win32VS2015")
        endif()
    elseif(MSVC_VERSION GREATER 1900)
        if(CMAKE_CL_64)
            set(MSVC_ARCH "x64Win64VS2017")
        else()
            set(MSVC_ARCH "i86Win32VS2017")
        endif()
    else()
        if(CMAKE_CL_64)
            set(MSVC_ARCH "x64Win64VSUnknown")
        else()
            set(MSVC_ARCH "i86Win32VSUnknown")
        endif()
    endif()
endmacro()

function(set_common_compile_options target)
    enable_language(C)
    enable_language(CXX)
    if(MSVC OR MSVC_IDE)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall
            -Wextra
            -Wshadow
            $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
            -pedantic
            -Wcast-align
            -Wunused
            $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
            -Wconversion
            -Wsign-conversion
            $<$<CXX_COMPILER_ID:GNU>:-Wlogical-op>
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<COMPILE_LANGUAGE:CXX>>:-Wuseless-cast>
            -Wdouble-promotion
            $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
            $<$<OR:$<AND:$<CXX_COMPILER_ID:GNU>,$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,6.0.0>>>,$<AND:$<C_COMPILER_ID:GNU>,$<NOT:$<VERSION_LESS:$<C_COMPILER_VERSION>,6.0.0>>>>:-Wnull-dereference>
            $<$<OR:$<AND:$<CXX_COMPILER_ID:GNU>,$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,7.0.0>>>,$<AND:$<C_COMPILER_ID:GNU>,$<NOT:$<VERSION_LESS:$<C_COMPILER_VERSION>,7.0.0>>>>:-Wduplicated-branches>
            $<$<OR:$<AND:$<CXX_COMPILER_ID:GNU>,$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,6.0.0>>>,$<AND:$<C_COMPILER_ID:GNU>,$<NOT:$<VERSION_LESS:$<C_COMPILER_VERSION>,6.0.0>>>>:-Wduplicated-cond>
            $<$<OR:$<AND:$<CXX_COMPILER_ID:GNU>,$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,7.0.0>>>,$<AND:$<C_COMPILER_ID:GNU>,$<NOT:$<VERSION_LESS:$<C_COMPILER_VERSION>,7.0.0>>>>:-Wrestrict>
            )
    endif()
endfunction()
