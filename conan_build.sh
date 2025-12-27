#!/bin/bash
set -e

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"
INSTALL_DIR="install"
JOBS="${2:-$(nproc)}"

echo "=== PlotJuggler Build Script ==="
echo "Build type: $BUILD_TYPE"
echo "Jobs: $JOBS"
echo ""

# Enable ccache
export PATH="/usr/lib/ccache:$PATH"
export CCACHE_DIR="${HOME}/.ccache"
export CMAKE_C_COMPILER_LAUNCHER=ccache
export CMAKE_CXX_COMPILER_LAUNCHER=ccache

# Run conan install if toolchain doesn't exist
if [ ! -f "${BUILD_DIR}/conan_toolchain.cmake" ]; then
    echo "=== Running Conan Install ==="
    conan install . --output-folder="${BUILD_DIR}" --build=missing -s build_type="${BUILD_TYPE}"
fi

# Configure with CMake
CMAKE_DIR="${BUILD_DIR}/${BUILD_TYPE}"

echo "=== Configuring with CMake ==="
echo "Build directory: ${CMAKE_DIR}"

cmake -S . -B "${CMAKE_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${BUILD_DIR}/conan_toolchain.cmake" \
    -DCMAKE_PREFIX_PATH="${BUILD_DIR}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

# Build
echo "=== Building ==="
cmake --build "${CMAKE_DIR}" --parallel "${JOBS}"

# Show ccache stats
echo ""
echo "=== ccache Statistics ==="
ccache -s

echo ""
echo "=== Build Complete ==="
