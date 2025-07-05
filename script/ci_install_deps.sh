#!/usr/bin/env bash

JOBS=4
BUILD_TYPE=Debug

set -ex

git clone https://github.com/red0124/doctest -b master --depth 1

cmake -S doctest -B doctest/build \
    -D CMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -D DOCTEST_WITH_MAIN_IN_STATIC_LIB=NO \
    -D DOCTEST_WITH_TESTS=NO

if [[ "${1}" == "sudo" ]]; then
    sudo cmake --build doctest/build --config ${BUILD_TYPE} --target install -j ${JOBS}
else
    cmake --build doctest/build --config ${BUILD_TYPE} --target install -j ${JOBS}
fi

rm -rf doctest
