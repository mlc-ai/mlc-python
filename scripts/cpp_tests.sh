#!/bin/bash
set -euxo pipefail

BUILD_REGISTRY=ON
BUILD_TYPE=RelWithDebInfo
BUILD_DIR=build-cpp-tests/
if [[ "$(uname)" == "Darwin" ]]; then
	NUM_PROCS=$(sysctl -n hw.ncpu)
else
	NUM_PROCS=$(nproc)
fi

cmake -S . -B ${BUILD_DIR} \
	-DMLC_BUILD_REGISTRY=${BUILD_REGISTRY} \
	-DMLC_BUILD_TESTS=ON \
	-DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build ${BUILD_DIR} \
	--config ${BUILD_TYPE} \
	--target mlc_tests \
	-j "${NUM_PROCS}"
GTEST_COLOR=1 ctest -V -C ${BUILD_TYPE} --test-dir ${BUILD_DIR} --output-on-failure
