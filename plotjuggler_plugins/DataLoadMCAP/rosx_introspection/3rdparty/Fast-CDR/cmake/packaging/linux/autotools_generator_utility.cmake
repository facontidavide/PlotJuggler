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

macro(generate_autotools_generator_script)
    get_property(${PROJECT_NAME}_SOURCES GLOBAL PROPERTY ${PROJECT_NAME}_SOURCES_PROPERTY)
    get_property(${PROJECT_NAME}_PUBLIC_HEADERS_DIRECTORIES GLOBAL PROPERTY ${PROJECT_NAME}_PUBLIC_HEADERS_DIRECTORIES_PROPERTY)
    get_property(${PROJECT_NAME}_PUBLIC_HEADERS_FILES GLOBAL PROPERTY ${PROJECT_NAME}_PUBLIC_HEADERS_FILES_PROPERTY)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/packaging/linux/autotools_generator.cmake.in ${PROJECT_BINARY_DIR}/cmake/packaging/linux/autotools_generator.cmake.in @ONLY)
    configure_file(${PROJECT_BINARY_DIR}/cmake/packaging/linux/autotools_generator.cmake.in ${PROJECT_BINARY_DIR}/cmake/packaging/linux/autotools_generator.cmake @ONLY)
endmacro()
