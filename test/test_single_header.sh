#!/bin/bash

set -x
set -e

TMP_HDR=test_single_header.hpp
TMP_SRC=test_single_header.cpp
TMP_BIN=test_single_header

python3 script/single_header_generator.py > ${TMP_HDR}
cat ${TMP_HDR} test/test_single_header_main.txt > ${TMP_SRC}

g++ -std=c++17 ${TMP_SRC} -o ${TMP_BIN} -Wall -Wextra
./${TMP_BIN}

rm ${TMP_HDR} ${TMP_SRC} ${TMP_BIN}
