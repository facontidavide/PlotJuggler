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

if(MSVC_VERSION LESS_EQUAL 1900)
    message(FATAL_ERROR "Visual Studio version ${MSVC_VERSION} is no longer supported")
else()
    if(CMAKE_CL_64)
        include("${CMAKE_CURRENT_LIST_DIR}/../share/fastcdr-x64Win64VS2019/cmake/fastcdr-config.cmake")
    else()
        include("${CMAKE_CURRENT_LIST_DIR}/../share/fastcdr-i86Win32VS2019/cmake/fastcdr-config.cmake")
    endif()
endif()

