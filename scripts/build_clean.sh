#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PJ_SRC="$(cd "${SCRIPT_DIR}/.." && pwd)"
PJ_DEV="$(cd "${PJ_SRC}/.." && pwd)"
PJ_DEPS="${PJ_DEV}/pj_local_deps"
QT_OVERLAY="${PJ_DEV}/qt_overlay/usr"
BUILD_DIR="${PJ_SRC}/build_cmake316_v3"
INSTALL_DIR="${PJ_SRC}/install_cmake316_v3"
BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"
JOBS="${JOBS:-$(nproc)}"

echo "[plotjuggler] source:  ${PJ_SRC}"
echo "[plotjuggler] deps:    ${PJ_DEPS}"
echo "[plotjuggler] qt:      ${QT_OVERLAY}"
echo "[plotjuggler] build:   ${BUILD_DIR}"
echo "[plotjuggler] install: ${INSTALL_DIR}"

rm -rf "${BUILD_DIR}" "${INSTALL_DIR}"

cmake -S "${PJ_SRC}" \
  -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
  -DCMAKE_PREFIX_PATH="${PJ_DEPS};${QT_OVERLAY}/lib/x86_64-linux-gnu/cmake;${QT_OVERLAY}" \
  -DCMAKE_LIBRARY_PATH="${PJ_DEPS}/lib;${QT_OVERLAY}/lib/x86_64-linux-gnu" \
  -DCMAKE_INCLUDE_PATH="${PJ_DEPS}/include;${QT_OVERLAY}/include/x86_64-linux-gnu/qt5" \
  -DBUILD_TESTING=OFF \
  -DCATKIN_ENABLE_TESTING=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_catkin=TRUE \
  -DCMAKE_DISABLE_FIND_PACKAGE_ament_cmake=TRUE \
  -DCMAKE_INSTALL_RPATH="${PJ_DEPS}/lib"

cmake --build "${BUILD_DIR}" --target plotjuggler -j"${JOBS}"
cmake --build "${BUILD_DIR}" --target install -j"${JOBS}"

echo "[plotjuggler] rebuild finished"
