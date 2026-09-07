#!/bin/bash
set -e

BUILD_TYPE="Debug"
BUILD_DIR="build"
TOOLCHAIN_FILE="${BUILD_DIR}/conan_toolchain.cmake"

rm -rf "${BUILD_DIR}"

mkdir -p "${BUILD_DIR}"

conan install . --output-folder="${BUILD_DIR}" --build=missing -s build_type="${BUILD_TYPE}"

if [[ ! -f "${TOOLCHAIN_FILE}" ]]; then
	echo "[setup] ERROR: Conan toolchain file not found at ${TOOLCHAIN_FILE}" >&2
	exit 1
fi
cmake -S . -B "${BUILD_DIR}" -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}"

