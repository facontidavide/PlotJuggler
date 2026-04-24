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
# Decoder options
# ============================================================
option(WITH_ALL_DECODERS "Enable all MCAP message decoders" OFF)
option(WITH_CDR "Enable CDR (ROS2) message decoding" OFF)
option(WITH_ROS1 "Enable ROS1 message decoding" OFF)
option(WITH_PROTOBUF "Enable Protobuf message decoding" OFF)

if(WITH_ALL_DECODERS)
    set(WITH_CDR ON)
    set(WITH_ROS1 ON)
    set(WITH_PROTOBUF ON)
endif()

# ============================================================
# Core SDK library
# ============================================================
add_subdirectory(src)

# ============================================================
# Tests (opt-in; gated by MOSAICO_INTEGRATION_TESTS inside)
# ============================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
