#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PJ_SRC="$(cd "${SCRIPT_DIR}/.." && pwd)"
PJ_DEV="$(cd "${PJ_SRC}/.." && pwd)"
PJ_DEPS="${PJ_DEV}/pj_local_deps"
BUILD_DIR="${PJ_SRC}/build_cmake316_v3"
BIN_PATH="${BUILD_DIR}/bin/plotjuggler"
PLUGIN_DIR="${BUILD_DIR}/bin"

if [[ ! -x "${BIN_PATH}" ]]; then
  echo "[plotjuggler] missing binary: ${BIN_PATH}" >&2
  echo "[plotjuggler] run scripts/build_clean.sh first" >&2
  exit 1
fi

export LD_LIBRARY_PATH="${PJ_DEPS}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

exec "${BIN_PATH}" \
  --plugin_folders "${PLUGIN_DIR}" \
  "$@"
