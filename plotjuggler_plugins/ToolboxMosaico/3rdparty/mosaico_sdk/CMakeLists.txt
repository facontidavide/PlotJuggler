cmake_minimum_required(VERSION 3.20)
project(MosaicoSDK CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies (Arrow, nlohmann_json) are provided by the parent project.
# When the SDK is add_subdirectory'd into PlotJuggler, the parent creates
# the `arrow::arrow` and `nlohmann_json::nlohmann_json` targets before
# reaching here. Calling find_package() here would fail on Conan-based
# builds (macOS, Windows) where nlohmann_json is provided as a CMake
# target but not a package config file.

add_subdirectory(src)
