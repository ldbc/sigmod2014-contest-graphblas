#!/usr/bin/env bash

# fail fast, echo command
set -e -x

# Build type: first argument, BUILD_TYPE env.var., "Release" (first existing option)
BUILD_TYPE=${1:-${BUILD_TYPE:-Release}}
BUILD_TYPE_LOWERCASE=$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')
PRINT_RESULTS=${PRINT_RESULTS:-1}
PRINT_EXTRA_COMMENTS=${PRINT_EXTRA_COMMENTS:-0}
CPP_DIR=$(dirname "$0")/..
CMAKE_BUILD_DIR=cmake-build-$BUILD_TYPE_LOWERCASE

cd "$CPP_DIR"
rm -rf "$CMAKE_BUILD_DIR"
mkdir "$CMAKE_BUILD_DIR"
cd "$CMAKE_BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DPRINT_RESULTS=$PRINT_RESULTS -DPRINT_EXTRA_COMMENTS=$PRINT_EXTRA_COMMENTS ..
make -j$(nproc)
