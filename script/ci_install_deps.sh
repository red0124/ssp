#!/bin/bash

JOBS=4
BUILD_TYPE=Debug

set -eux

git clone https://github.com/red0124/doctest -b master --depth 1

cmake -S doctest -B doctest/build \
    -D CMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -D DOCTEST_WITH_MAIN_IN_STATIC_LIB=NO \
    -D DOCTEST_WITH_TESTS=NO

cmake --build doctest/build --config ${BUILD_TYPE} --target install -j ${JOBS}

rm -rf doctest
