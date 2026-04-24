cmake_minimum_required(VERSION 3.20)
project(MosaicoSDK CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Dependencies (provided by parent project)
# ============================================================
find_package(Arrow REQUIRED)
find_package(nlohmann_json REQUIRED)

# ============================================================
# Core SDK library
# ============================================================
add_subdirectory(src)
